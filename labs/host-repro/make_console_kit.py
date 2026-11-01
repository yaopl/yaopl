"""Build console test files for the parser fixes from one bootable PS2 ISO.

Usage: make_console_kit.py <bootable.iso> <out dir>

The ISO must have SYSTEM.CNF;1 in its root and fit its root directory in one sector, e.g. a
homebrew ELF packed as a disc. Every variant keeps that ISO's game ID. Cheat files only use
codes with address 0 and no hook line (9xxxxxxx): without a hook the cheat engine never
applies the codes it is given.
"""
import os
import shutil
import struct
import sys
import zlib

SECTOR = 2048


def usba_crc32(name: bytes) -> int:
    """USBA_crc32 with unsigned bytes, as OPL computes it after the fix."""
    tab = [0] * 256
    crc = 0
    for t in range(256):
        crc = (t << 24) & 0xFFFFFFFF
        for _ in range(8):
            crc = (crc << 1) & 0xFFFFFFFF if crc & 0x80000000 else ((crc << 1) ^ 0x04C11DB7) & 0xFFFFFFFF
        tab[255 - t] = crc
    for b in (name + b"\0")[:33]:
        crc = tab[b ^ (crc >> 24)] ^ ((crc << 8) & 0xFFFFFF00)
    return crc


def root_dir(iso: bytearray):
    pvd = 16 * SECTOR
    assert iso[pvd] == 1 and iso[pvd + 1 : pvd + 6] == b"CD001"
    lba, size = struct.unpack_from("<I", iso, pvd + 156 + 2)[0], struct.unpack_from("<I", iso, pvd + 156 + 10)[0]
    assert size == SECTOR, "root directory must be one sector"
    records, pos = [], lba * SECTOR
    while iso[pos] != 0:
        records.append(bytes(iso[pos : pos + iso[pos]]))
        pos += iso[pos]
    return lba, records


def record(name: bytes, lba: int, size: int, xa: int = 0) -> bytes:
    length = 33 + len(name) + (1 - len(name) % 2)  # ISO 9660 9.1.12: pad to an even length
    r = bytearray(length)
    r[0], r[1] = length, xa
    struct.pack_into("<I", r, 2, lba)
    struct.pack_into(">I", r, 6, lba)
    struct.pack_into("<I", r, 10, size)
    struct.pack_into(">I", r, 14, size)
    r[32] = len(name)
    r[33 : 33 + len(name)] = name
    return bytes(r)


def with_record_after_dotdot(base: bytes, extra: bytes) -> bytes:
    """Insert a record right after '.' and '..', before SYSTEM.CNF;1, as OPL scans in order."""
    iso = bytearray(base)
    lba, records = root_dir(iso)
    sector = b"".join(records[:2] + [extra] + records[2:])
    assert len(sector) <= SECTOR
    iso[lba * SECTOR : (lba + 1) * SECTOR] = sector.ljust(SECTOR, b"\0")
    return bytes(iso)


def system_cnf(iso: bytes):
    _, records = root_dir(bytearray(iso))
    for r in records:
        if r[33 : 33 + r[32]] == b"SYSTEM.CNF;1":
            return struct.unpack_from("<I", r, 2)[0], struct.unpack_from("<I", r, 10)[0]
    raise SystemExit("no SYSTEM.CNF;1 in the root")


def game_id(iso: bytes) -> bytes:
    lba, size = system_cnf(iso)
    cnf = iso[lba * SECTOR : lba * SECTOR + size]
    value = cnf.split(b"BOOT2", 1)[1].split(b"=", 1)[1].strip().split(b"\r")[0].split(b"\n")[0]
    return value.split(b":", 1)[1].lstrip(b"\\").split(b";")[0]


def zso(iso: bytes, align: int = 0) -> bytes:
    """ZSO with every block stored uncompressed (index bit 31 set)."""
    blocks = len(iso) // SECTOR
    data = 24 + 4 * (blocks + 1)
    index = [((data + k * SECTOR) >> align) | 0x80000000 for k in range(blocks)] + [(data + blocks * SECTOR) >> align]
    header = b"ZISO" + struct.pack("<IQIBB2x", 24, len(iso), SECTOR, 1, align)
    return header + struct.pack("<%dI" % len(index), *index) + iso


def zso_blocks(iso: bytes, block_size: int) -> bytes:
    """ZSO with uncompressed blocks of block_size bytes, as maxcso writes 16 KiB blocks by default
    for images of 2 GiB and more. The image is padded to whole blocks."""
    iso = iso + b"\0" * ((-len(iso)) % block_size)
    blocks = len(iso) // block_size
    data = 24 + 4 * (blocks + 1)
    index = [(data + k * block_size) | 0x80000000 for k in range(blocks)] + [data + blocks * block_size]
    header = b"ZISO" + struct.pack("<IQIBB2x", 24, len(iso), block_size, 1, 0)
    return header + struct.pack("<%dI" % len(index), *index) + iso


def zso_bad_index(z: bytes, sector: int) -> bytes:
    """Swap the index entries of `sector` and the next one: the sector then ends before it starts."""
    z = bytearray(z)
    a, b = 24 + 4 * sector, 24 + 4 * (sector + 1)
    z[a : a + 4], z[b : b + 4] = z[b : b + 4], z[a : a + 4]
    return bytes(z)


def png_grey(width: int, height: int, bad_after: int = 0) -> bytes:
    """An 8-bit greyscale PNG (colour type 0) with a diagonal gradient. With bad_after, the zlib data
    holds that many rows and then a block of the reserved type 3, which libpng fails on."""

    def chunk(tag: bytes, data: bytes) -> bytes:
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    rows = b"".join(b"\0" + bytes((x + y) & 0xFF for x in range(width)) for y in range(height))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    idat = zlib.compress(rows)
    if bad_after:
        first = rows[: bad_after * (1 + width)]
        idat = b"\x78\x01\x00" + struct.pack("<HH", len(first), len(first) ^ 0xFFFF) + first + b"\x07" + bytes(16)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat) + chunk(b"IEND", b"")


THEME_CFG = """# YAOPL test theme: no ItemsList (OPL adds a default one), a StaticImage whose file is missing,
# a GameCountText and a wrapped Description
main0:
\ttype=Background
\tpattern=BG
main1:
\ttype=StaticImage
\tdefault=missing_image
\tx=420
\ty=40
main2:
\ttype=GameCountText
\tx=420
\ty=60
main3:
\ttype=AttributeText
\tattribute=Description
\tx=420
\ty=90
\twidth=200
\theight=300
\twrap=1
main4:
\ttype=ItemText
\tx=-124
\ty=-155
main5:
\ttype=HintText
\taligned=1
\tx=POS_MID
"""


def ul_record(name: bytes, startup: bytes) -> bytes:
    r = bytearray(64)
    r[0 : len(name)] = name
    r[0x20:0x23] = b"ul."
    r[0x23 : 0x23 + len(startup)] = startup
    r[0x2F] = 1  # parts
    r[0x30] = 0x14  # DVD
    r[0x35] = 0x08
    return bytes(r)


def write(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def main():
    base = open(sys.argv[1], "rb").read()
    out = sys.argv[2]
    gid = game_id(base)
    cnf_lba, cnf_size = system_cnf(base)
    dvd = os.path.join(out, "stick", "DVD")

    write(os.path.join(dvd, "T0 control.iso"), base)
    write(os.path.join(dvd, "T1 zero-length name.iso"), with_record_after_dotdot(base, record(b"", cnf_lba, cnf_size)))
    write(os.path.join(dvd, "T2 ext-attr byte.iso"), with_record_after_dotdot(base, record(b"XA.TXT;1", cnf_lba, cnf_size, xa=0x80)))
    write(os.path.join(dvd, "T3 long file name.iso"), with_record_after_dotdot(base, record(b"A_VERY_LONG_LEVEL2_FILENAME.TXT;1", cnf_lba, cnf_size)))
    no_eq = bytearray(base)
    start = cnf_lba * SECTOR
    cnf = bytes(no_eq[start : start + cnf_size])
    i = cnf.index(b"BOOT2") + 5
    j = cnf.index(b"=", i)
    no_eq[start + j] = ord(" ")
    write(os.path.join(dvd, "T4 BOOT2 without equals.iso"), bytes(no_eq))
    control_zso = zso(base, 0)
    write(os.path.join(dvd, "T5 control.zso"), control_zso)
    write(os.path.join(dvd, "T6 align 12.zso"), control_zso[:21] + bytes([12]) + control_zso[22:])
    write(os.path.join(dvd, "T10 16k blocks.zso"), zso_blocks(base, 16384))
    # The index of the SYSTEM.CNF sector is corrupt: reading it must fail, not write before the buffer
    write(os.path.join(dvd, "T11 bad index.zso"), zso_bad_index(control_zso, cnf_lba))

    # One record plus one stray byte; the chunk is the control image.
    name = b"T7 ragged ul.cfg"
    write(os.path.join(out, "ul-ragged", "ul.cfg"), ul_record(name, gid) + b"\0")
    write(os.path.join(out, "ul-ragged", "ul.%08X.%s.00" % (usba_crc32(name), gid.decode())), base)

    for name in (b"T8 ul ASCII title", "T9 Тест кириллицы".encode("utf-8")):
        assert len(name) <= 32
        write(os.path.join(out, "ul-cyrillic", "ul.%08X.%s.00" % (usba_crc32(name), gid.decode())), base)
    write(
        os.path.join(out, "ul-cyrillic", "ul.cfg"),
        ul_record(b"T8 ul ASCII title", gid) + ul_record("T9 Тест кириллицы".encode("utf-8"), gid),
    )

    cht = gid.decode() + ".cht"
    write(
        os.path.join(out, "cht-indented", cht),
        b'"Indented comment test"\r\nMastercode\r\n00000000 00000001\r\n\r\n'
        b" //An indented comment\r\n00000000 00000002\r\n\r\nCheat B\r\n00000000 00000003\r\n",
    )
    write(os.path.join(out, "cht-251", cht), b"".join(b"Cheat %d\r\n00000000 %08X\r\n" % (n, n + 1) for n in range(251)))
    write(os.path.join(out, "cht-long-last-line", cht), b"Cheat A\r\n00000000 00000001\r\nCheat B\r\n00000000 00000002\r\n" + b"X" * 400)
    cht_dup = (
        b"Mastercode\r\n00000000 00000001\r\n"
        b"Character A\r\nInfinite Health\r\n00000000 00000002\r\n"
        b"Character B\r\nInfinite Health\r\n00000000 00000003\r\n"
    )
    write(os.path.join(out, "cht-duplicate-names", cht), cht_dup)

    # The game's own CFG, ending with a comment line without LF
    write(os.path.join(out, "cfg-comment-end", gid.decode() + ".cfg"), b"$Compatibility=0\r\n#end")
    # A VMC of a size OPL's menu does not offer (contents do not matter for the menu)
    write(os.path.join(out, "vmc-4mb", "keep4mb.bin"), bytes(4 * 1024 * 1024))

    # A Description of 255 characters, the most the config reader keeps, for the test theme's wrap
    description = b"".join(b"Word%03d " % n for n in range(40))[:255]
    write(os.path.join(out, "cfg-long-description", gid.decode() + ".cfg"), b"Title=Long description test\r\nDescription=" + description + b"\r\n")
    write(os.path.join(out, "theme-test", "THM", "thm_yaopl-test", "conf_theme.cfg"), THEME_CFG.encode("ascii"))
    # A grey cover: OPL only loaded RGB, RGBA and palette PNGs
    write(os.path.join(out, "art-grey", "ART", gid.decode() + "_COV.png"), png_grey(140, 200))
    # Broken art: a cover cut short, an icon whose image data turns bad after 8 rows, a screenshot
    # 100000 pixels wide. Each must fail to load and leave the game without that image; OPL read past
    # the cut cover, lost the memory of the first two and allocated about 800 KB of rows for the third
    cover = png_grey(140, 200)
    write(os.path.join(out, "art-broken", "ART", gid.decode() + "_COV.png"), cover[: len(cover) // 2])
    write(os.path.join(out, "art-broken", "ART", gid.decode() + "_ICO.png"), png_grey(64, 64, bad_after=8))
    write(os.path.join(out, "art-broken", "ART", gid.decode() + "_SCR.png"), png_grey(100000, 1))
    # A language file name shorter than lang_X.lng: the scan of a device's LNG folder crashed on it
    write(os.path.join(out, "lng-short-name", "LNG", "ru.lng"), b"Russian\r\nFontName\r\n")
    print("game ID", gid.decode(), "- files written to", out)


main()
