"""Write the malformed inputs used by run_all.ps1 into the given directory."""
import os
import struct
import sys
import zlib


def cheats(out):
    files = {
        "cht_plain.cht": b"Mastercode\r\n90106418 0c0418ae\r\n\r\nCheat\r\n20100000 00000001\r\n",
        # Same layout as SLES_505.41.cht in PS2-Widescreen/OPL-Widescreen-Cheats
        "cht_indented.cht": b'"Game /ID SLES_000.00"\r\nMastercode\r\n90106418 0c0418ae\r\n\r\n'
        b" //Gameplay 16:9\r\n20142e50 3c023faa\r\n201430c4 3c023faa\r\n",
        "cht_251.cht": b"".join(b"Cheat %d\r\n2%07X 00000001\r\n" % (n, 0x100000 + n * 4) for n in range(251)),
        "cht_long_last.cht": b"Mastercode\r\n90106418 0c0418ae\r\n" + b"A" * 400,
        "cht_no_lf.cht": b"Mastercode\r\n90106418 0c0418ae\r\nCheat\r\n20100000 00000001",
        # The cheat of upstream issue #1548: lines with address 0 are the second half of multi-line codes
        "cht_addr0.cht": b"Mastercode\r\n90183688 0C07B869\r\n\r\nAiming on L3, off on R3\r\n"
        b"E10100FD 0028489C\r\n000FFFE0 00000001\r\nE10100FB 0028489C\r\n000FFFE0 00000000\r\n"
        b"E1020000 000FFFE0\r\n60284390 00000080\r\n00000000 00000B4C\r\n"
        b"E1020001 000FFFE0\r\n60284390 00000088\r\n00000000 00000B4C\r\n"
        b"E1010002 2028C598\r\n000FFFE0 00000000\r\n\r\nAnother cheat\r\n20100000 00000001\r\n",
        # Same shape as SLUS_210.09.cht (Sega Classics Collection): a Mastercode with 9 hook lines
        "cht_9hooks.cht": b"Mastercode\r\n" + b"".join(b"9%07X 0C047714\r\n" % (0x01B9920 + n * 0x100) for n in range(9))
        + b"\r\nCheat\r\n2039FCF8 3FAAAAAB\r\n",
        # Cheat names in Cyrillic (UTF-8): one followed by a comment, one with no ASCII at all, one with ASCII
        "cht_utf8.cht": b"Mastercode\r\n90106418 0c0418ae\r\n"
        + "Бессмертие // by X\r\n20142E50 00000001\r\n".encode()
        + "Деньги\r\n20142E60 00000002\r\n".encode()
        + "Макс (P1)\r\n20142E70 00000003\r\n".encode(),
        # 100 cheats of 3 codes: 300 codes for the engine's list of 250 when all are enabled
        "cht_many.cht": b"".join(
            b"Cheat %d\r\n2%07X 00000001\r\n2%07X 00000002\r\n2%07X 00000003\r\n" % (n, 0x100000 + n * 16, 0x100004 + n * 16, 0x100008 + n * 16)
            for n in range(100)
        ),
    }
    for name, data in files.items():
        with open(os.path.join(out, name), "wb") as f:
            f.write(data)


def ul_record(name, startup):
    r = bytearray(64)
    r[0 : len(name)] = name
    r[0x20:0x23] = b"ul."
    r[0x23 : 0x23 + len(startup)] = startup
    r[0x2F] = 2
    r[0x30] = 0x14
    r[0x35] = 0x08
    return bytes(r)


def ul_cfg(out):
    a = ul_record(b"Game A", b"SLUS_200.62")
    b = ul_record(b"Game B", b"SLES_505.41")
    for d, data in (("ul64", a), ("ul65", a + b"\x00"), ("ul129", a + b + b"\x0a")):
        os.makedirs(os.path.join(out, d), exist_ok=True)
        with open(os.path.join(out, d, "ul.cfg"), "wb") as f:
            f.write(data)
    os.makedirs(os.path.join(out, "cachedir"), exist_ok=True)


def system_cnf(out):
    for name, boot2 in (("cnf_ok.cnf", b"BOOT2 = "), ("cnf_no_eq.cnf", b"BOOT2 ")):
        with open(os.path.join(out, name), "wb") as f:
            f.write(boot2 + b"cdrom0:\\SLUS_200.62;1\r\nVER = 1.00\r\nVMODE = NTSC\r\n")


def config_files(out):
    files = {
        "cfg_comment_end.cfg": b"a=1\r\n#end",  # last line is a comment without LF
        "cfg_plain.cfg": b"a=1\r\n#comment\r\nb=2\r\n",
        "IPCONFIG.DAT": b"192.168.0.10 255.255.255.0 192.168.0.1".ljust(300, b" "),
    }
    for name, data in files.items():
        with open(os.path.join(out, name), "wb") as f:
            f.write(data)


def lz4_run(byte, size):
    """One LZ4 block of `size` copies of `byte`: a literal, a match on it and 11 closing literals."""
    ext = (size - 12) - 4 - 15  # match length beyond the token's 15, written as 255s and a remainder
    block = bytearray([0x1F, byte, 1, 0])
    while ext >= 255:
        block.append(255)
        ext -= 255
    block.append(ext)
    block.append(0xB0)
    block += bytes([byte]) * 11
    return bytes(block)


def sector_byte(n):
    return (n * 7 + 1) & 0xFF


def zso_file(path, sectors, block_size=2048, align=0, raw_all=False, swap=None):
    """A ZSO image whose sector n holds 2048 bytes of sector_byte(n). Even blocks are LZ4 (one run),
    odd blocks raw. swap=(a, b) exchanges two index entries, so block a ends before it starts."""
    per_block = block_size // 2048
    nblocks = sectors * 2048 // block_size
    unit = 1 << align
    blocks = []
    for b in range(nblocks):
        data = b"".join(bytes([sector_byte(s)]) * 2048 for s in range(b * per_block, (b + 1) * per_block))
        if raw_all or b % 2:
            blocks.append((True, data))
        else:
            blocks.append((False, lz4_run(sector_byte(b), 2048)))
    first = 24 + 4 * (nblocks + 1)
    pos = (first + unit - 1) // unit * unit
    index, body = [], b""
    for raw, data in blocks:
        index.append((pos // unit) | (0x80000000 if raw else 0))
        padded = data + b"\0" * ((-len(data)) % unit)
        body += padded
        pos += len(padded)
    index.append(pos // unit)
    if swap:
        index[swap[0]], index[swap[1]] = index[swap[1]], index[swap[0]]
    header = struct.pack("<IIQIBBBB", 0x4F53495A, 24, sectors * 2048, block_size, 1, align, 0, 0)
    with open(path, "wb") as f:
        f.write(header + struct.pack("<%dI" % len(index), *index) + b"\0" * ((-first) % unit) + body)


def zso_images(out):
    zso_file(os.path.join(out, "zso_ok.zso"), 400)
    zso_file(os.path.join(out, "zso_align4.zso"), 40, align=4)
    zso_file(os.path.join(out, "zso_badidx.zso"), 10, swap=(3, 4))
    # maxcso writes 16 KiB blocks by default for images of 2 GiB and more
    zso_file(os.path.join(out, "zso_16k.zso"), 64, block_size=16384, raw_all=True)


def msgbox_texts(out):
    texts = {
        # the read-only UDPBD VMC note: in Russian, and in Arabic (drawn right to left)
        "msg_ru.txt": "Этот сервер UDPBD только "
        "для чтения, поэтому VMC "
        "на нём использовать "
        "нельзя; игра сохраняет "
        "на настоящую карту "
        "памяти.",
        "msg_ar.txt": "هذا الخادم للقراءة "
        "فقط، لذلك لا يمكن UDPBD "
        "استخدام VMC عليه؛ تحفظ "
        "اللعبة على بطاقة "
        "الذاكرة الحقيقية. " * 2,
        # longer than the message box buffer
        "msg_long.txt": "word " * 400,
    }
    for name, text in texts.items():
        with open(os.path.join(out, name), "wb") as f:
            f.write(text.encode("utf-8"))


def png_chunk(kind, data):
    return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))


def png_file(width, height, color_type, depth, idat, plte=None, trns=None):
    ihdr = struct.pack(">IIBBBBB", width, height, depth, color_type, 0, 0, 0)
    data = b"\x89PNG\r\n\x1a\n" + png_chunk(b"IHDR", ihdr)
    if plte:
        data += png_chunk(b"PLTE", plte)
    if trns:
        data += png_chunk(b"tRNS", trns)
    return data + png_chunk(b"IDAT", idat) + png_chunk(b"IEND", b"")


def png_rows(width, height, bpp, seed=1):
    # pseudo-random pixels, so that the compressed image is about as large as the raw one
    rows, x = b"", seed
    for _ in range(height):
        row = bytearray()
        for _ in range((width * bpp + 7) // 8):
            x = (x * 1103515245 + 12345) & 0x7FFFFFFF
            row.append(x >> 16 & 0xFF)
        rows += b"\x00" + bytes(row)
    return rows


def png_images(out):
    rows = png_rows(64, 48, 32)
    rgba = png_file(64, 48, 6, 8, zlib.compress(rows))
    first = rows[: 8 * (1 + 64 * 4)]
    files = {
        "png_rgba.png": rgba,
        "png_pal4.png": png_file(32, 16, 3, 4, zlib.compress(png_rows(32, 16, 4)),
                                 plte=bytes(range(48)), trns=bytes(range(0, 256, 16))),
        "png_pal8.png": png_file(32, 16, 3, 8, zlib.compress(png_rows(32, 16, 8)),
                                 plte=bytes(n % 256 for n in range(768, 0, -1))),
        # the file ends in the middle of the image data
        "png_trunc.png": rgba[: len(rgba) // 2],
        # complete chunks, but after 8 rows the zlib stream has a block of the reserved type 3
        "png_badzlib.png": png_file(64, 48, 6, 8, b"\x78\x01\x00" + struct.pack("<HH", len(first), len(first) ^ 0xFFFF)
                                    + first + b"\x07" + bytes(16)),
        # far wider than the 1024 pixels OPL takes
        "png_wide.png": png_file(1000000, 1, 6, 8, zlib.compress(bytes(1 + 4000000), 9)),
        # as wide as OPL takes
        "png_1024.png": png_file(1024, 4, 2, 8, zlib.compress(png_rows(1024, 4, 24))),
    }
    for name, data in files.items():
        with open(os.path.join(out, name), "wb") as f:
            f.write(data)


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    cheats(out)
    ul_cfg(out)
    system_cnf(out)
    config_files(out)
    zso_images(out)
    msgbox_texts(out)
    png_images(out)
    with open(os.path.join(out, "zero.bin"), "wb") as f:
        f.write(bytes(65536))
    print("inputs written to", out)


main()
