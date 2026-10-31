# Changelog

What changed in YAOPL (Yet Another Open PS2 Loader), newest first; below it, what changed in Open PS2
Loader (OPL) before the fork.

## Unreleased

- Apps: An app started with an argument (`argv1=` in its `title.cfg`) gets it intact: it was read from memory
  already freed when the menu closed. (e861b52e)
- Apps: The apps list no longer loses memory each time it is built again, at start and whenever a USB drive
  comes or goes or another page loads. (7dd0e924, e53856db)
- GUI: A game list that does not fit in memory no longer loses the memory of every game found on each rescan.
  (33cbfb42)
- GUI: The settings dialogs, the on-screen keyboard and the colour picker no longer draw a theme or language
  that was replaced while they were open (a USB stick, SMB share or HDD with the configured theme or language
  coming up, or the stick with the language pulled): the menu could show garbled text or freeze. (ce6323f5)
- GUI: After a 720p, 1080i or other HD video mode, the menu no longer runs above the background music and the
  cover loading until the next start: leaving the mode gives the menu its normal priority back. (9ab1f447)
- GUI: The menu no longer freezes now and then when a theme or language font loads while text is drawn, for
  example when the USB drive with the chosen theme comes up after start. (74a6607a)
- GUI: Changing the theme or language while covers load no longer damages memory, which could crash the menu
  later; covers asked for while the NBD server runs load afterwards instead of never. (20511b72)
- GUI: Display Settings no longer shows or saves garbage when a device with themes or languages comes or goes
  while it is open; a theme or language picked from a device that has gone is not applied. (a0d72638)
- GUI: A theme image whose `default` names no built-in picture is left out instead of crashing the menu.
  (9712bc26)
- PADEMU: A DualShock 4 on USB gets its LED colour and vibration from the first connection in a game, not only
  after it is plugged in again. (96f8f0c2)
- SMB: A game over SMB whose connection broke (for example after Samba dropped an idle PS2) no longer blocks
  every later disc read and IGR: a failed read or write releases the SMB lock, and a connection closed in the
  middle of a reply ends the read instead of looping forever. IGR then gets back to YAOPL. (f2c3a673,
  e16e2d15)
- SMB: A VMC over SMB reports a failed read or write, or a card that cannot be opened, to the game as an error
  instead of as a page done. (964e4178)
- SMB: The [SMB share](https://yaopl.github.io/yaopl/devices/smb/#a-game-freezes-after-an-idle-time) page
  gives the Samba settings that keep a game from freezing after the console was left idle (upstream issue
  #79). (36e07200)
- GUI: Exiting, powering off or starting a game no longer hangs for good when the menu still has work queued,
  such as a cover loading over the network or an SMB start without a cable. (b5394714)
- GUI: A theme that does not ship every button icon of the hint bar no longer freezes the menu; such a hint
  shows its text without the icon. (85500f43)
- In-game: With an HD Pro adaptor, a failed disc transfer no longer leaves the drive locked, which hung the
  game at its next load. (639934bf)
- GUI: A theme's or the default `bgm.ogg` that is not an Ogg stream no longer leaves a file open each time the
  music starts. (9343404d)
- GUI: A font file that cannot be read whole (its device removed during the read) is no longer used half read.
  (98c6ea94)
- GUI: Switching from a language to English, or losing the device with the language file, frees the
  translation's memory. (dea645fa)
- GUI: Removing a USB drive (or deleting a game) while the cursor rests on a game no longer crashes the menu
  when that game's settings were still loading. (c0d9b51e)
- GUI: The device pages added to the menu at start can no longer, in a rare timing, freeze it. (d754bb56)
- GUI: The menu no longer freezes now and then right after start, when the games of a USB device appear, or
  when a device with themes or languages is connected; it happened most with large themes and a DualShock 3.
  (b45a592d)
- Settings: A save of the settings to a USB drive that has none yet no longer leaves an empty settings file
  there when the save fails; the next start used it and started with the defaults instead of the memory card's
  settings. (3ceb3d42)
- GUI: After a change of the BDM prefix or the MMCE slot or folder, the themes and languages of the new folder
  are listed and the old folder's leave the lists, without a restart. (e9cd2a61)
- GUI: The size on a game's info page is read from the path the game starts from; under a BDM or SMB prefix
  and on an MMCE card the path had a doubled separator, which a device may not accept. (50303f58)
- Settings: A game started by another program reads YAOPL's settings in the same order as the menu, the device
  YAOPL was started from first. (915c52db)
- Settings: Settings saved on a USB drive when they could not be saved where they were read go to the root of
  the drive, where they are found at the next start, also with a BDM prefix. (f2fef874)
- Settings: A settings file saved into a folder that is not there yet makes the folder on any device, an SMB
  share included, not only on a memory card: the settings of a game on a read-only UDPBD server save when
  YAOPL's settings are in the root of a USB drive or an MMCE card with no `CFG` folder there. (ea75aa4b,
  133a091e, 15d698c6)
- Settings: When the settings cannot be saved where they were read, all their files are saved in the next
  place, not only the changed ones, and a failed save is no longer reported as saved. (f34274dd, f4b08859)
- Settings: Started from an MMCE card, YAOPL keeps its settings in the root of the card's SD card, which does
  not change with the card's memory card. (1328f72b)
- Settings: The device YAOPL was started from comes first for its settings, before the memory card, and holds
  them from the first save; its themes and languages are then used first too. (d861fb5e)
- Settings: YAOPL no longer saves its settings on MX4SIO, iLink or a UDPBD server, where they were not found
  at the next start; Save Changes reports an error if there is no other place. (2d5adc29)
- Settings: Started from the HDD (or from any folder of a USB drive), YAOPL recognises the device it was
  started from; an HDD start read the USB drive's settings before the HDD's. (be29a311)
- GUI: Themes and languages of a USB drive or another device that goes away leave the lists; the one in use
  falls back to another copy, or to the built-in theme and English until the device is back. A network share
  that reconnects no longer resets a theme from elsewhere or lists its languages twice. (6c8d4f31)
- GUI: A theme or language on several devices is listed once; the copy on the device with the settings is
  used, then the memory card's. (2bd5b1e0)
- GUI: Saving the settings before the device with the chosen theme or language is found no longer replaces
  them with the internal theme and English; a theme picked but not saved yet is no longer switched back when
  a device is found. (6bc22612)
- GUI: The colour picker of the theme settings shows the value of each channel, 0 to 255, next to its bar.
  (faaf2b61)
- GUI: A new text colour of the internal theme reaches the app list and the info screen at once, not only
  after a restart. (3fb0e53a)
- GUI: The About page shows the whole version, with the commit and `-yaopl` at the end, and all build options;
  both were cut to 39 characters. (9847f4ca)
- GUI: Long messages wrap onto several lines inside the message box instead of running off the screen, in
  every language and video mode; short messages look as before. The same goes for the *Please wait* screen,
  warnings, the video mode confirmation and the prompt for removing game settings. (6a091fc2)

## 2.0.0-rc2 (2026-10-29)

- UDPBD: When the UDPBD device connects, YAOPL checks with a small test file whether the server keeps what is
  written: read-only servers (udpbd-vexfat, PS2-Servers with a folder or `--read-only`) do not say so. A write
  that PS2-Servers refuses no longer makes the whole served disk unreadable until YAOPL is restarted.
  (df65546e, 093b541b)
- UDPBD: With a read-only UDPBD server, game settings can be saved: they go to a `CFG` folder next to YAOPL's
  own settings, usually `mc0:OPL/CFG/`, instead of the served disk, where saving failed (*Error writing
  settings!*) or was lost. A `CFG` file on the served disk is still used until the game has settings saved
  there. With a server that writes, settings stay on the served disk, as on USB. (7781959c)
- UDPBD: With a read-only UDPBD server, the game menu has no Rename and Delete, and *Configure VMC* shows a
  note instead of *Failed to format VMC file*; a VMC set in a game's settings is not used and the game saves
  to the real memory card, without a question at every start. (9f556b55, 11037583, 7c375a09)
- UDPBD: Nothing but the write check's test file is written to a read-only UDPBD server: YAOPL does not keep
  its game list cache (`games.bin`) there, does not offer Rename and Delete for apps in its `APPS` folder, and
  does not pick it when it looks for a device to save its own settings on. (c92f681d, afe59be2, 760d7e3c)
- In-game: IGR no longer stalls games while their pad polling pauses (loading screens, movies): it scanned the
  game's memory again every time, which left the DualShock dead or dropping out in Gran Turismo 4 over UDPBD
  until it was replugged. Gran Turismo 4 no longer needs Compatibility Mode 6 for its pad. After patching a
  game's pad calls IGR also flushes the caches. (3bdfc94f, fd0af6aa; from RiptOPL)
- PADEMU: IGR from a game with a DualShock 3/4 on USB PADEMU no longer hangs before the PS2 browser or the
  exit ELF comes up: the USB host controller is stopped before the IOP reset. (e72c9f48)
- Translations: The message about the fragment count of a VMC, its defragment hint and the VMC note for a
  read-only UDPBD server are translated into Russian; in the other languages they show in English until
  translated. (yaopl-lang 23f4261)
- UDPBD: A game over UDPBD no longer hangs at its first screen when a USB device is plugged in: the USB host
  controller that the menu's USB driver left running is stopped. (15eee615)
- UDPBD: A game over UDPBD no longer waits forever for the disk when the first request to the server went out
  before the network link was up: the request is repeated until the server answers. (68de38f8)
- PADEMU: A DualShock 3/4 on USB PADEMU reads its buttons more reliably when it is slow to answer, as a
  DualShock 3 plugged in after the game has started: no more input lag and uneven LED blinking in that case.
  Its start command and LED/vibration report no longer share a buffer with the button reports. (1ef5b49c,
  4b1106e4, c4a82e95)
- MMCE: With "Send the game ID" on, a game starts only after the MMCE card has switched to the game's memory
  card (up to 7.5 s): an SD2PSX does not answer while it swaps the cards, and YAOPL took that silence for
  "done" and started the game about 0.3 s after the command, while the card was still switching. (19578cc6)
- MMCE: The choices of IGR Boot Card Slot(s) are translated: "Off" (was "None") and "Both" were fixed English
  text. (999bb6d3)
- VMC: A fragmented VMC on a BDM device (USB, MX4SIO, iLink, exFAT HDD), as exFAT often makes them (upstream
  issue #1695), is reported with the number of its fragments: *VMC ... is in N fragments. Continue with Memory
  Card in slot ...?*. Backing out says how to make the file one piece: move it to a PC and copy it back. The
  game still cannot use such a card. (c19eed08)
- GUI: The device bar of the default theme shows all five pages, MMCE included, on every page: before, MMCE
  appeared only when it was selected and the bar jumped from four icons to five. Icons by Berion. (5776873f)
- GUI: The dots above the device bar show as many dots as mass devices are connected, with the current one
  lit: before, there were always five, one per device slot, slots 0 and 2 left the first device without dots
  and a single device in slot 2 still got them. Themes with their own five-dot pictures keep them. (f84b227d)
- Game settings: Game ID → Load From Disc works more than once: before, every try after the first said *Error
  while loading the Game ID* until YAOPL was restarted, and the drive kept spinning after the first read
  (upstream issue #651). (7f1018e7)
- Translations: The new "Both" is translated into Russian; in the other languages it shows in English until
  translated. (yaopl-lang 33afafa, 5985982)

## 2.0.0-rc1 (2026-10-28)

The first release of the fork, on top of upstream OPL Beta-2271 (b3a21997).

- UDPBD: New BDM device UDPBD (Settings → BDM Devices → UDPBD, off by default): games on a disk or disk image
  that a PC serves over the LAN (udpbd-server, udpbd-vexfat, PS2-Servers) are listed as "UDPBD Games" and run
  in the OPL core with all its features. The PS2 uses the static IP of the network settings; while UDPBD is
  on, SMB and the other network features do not start. Modules built from `rickgaiser/neutrino`. (83c63032,
  c6bec984, b3ff1093)
- UDPBD: UDPBD asks for at most 16 KiB per read: longer bursts of reply packets were lost on a real console
  and hung the game. (12cc6fb5)
- UDPBD: With the HDD (APA) page on, the ATA driver is loaded before UDPBD, so its reset of the DEV9 ATA
  interface no longer cuts UDPBD off. (2a79d23b)
- UDPBD: The UDPBD device has its own icon instead of the generic one. (be2dbef1)
- In-game: The SDK `sio2man` goes into a game only when a VMC, PadEmu, MX4SIO or MMCE needs it; other games
  keep their own SIO2MAN and pad and memory-card drivers, as before upstream Beta-2248 (#1785). Suspected
  cause of games that freeze after some minutes while the sound goes on (upstream issues #1795, #1790); not
  yet confirmed on a console. (5ef1b9e2)
- HDD: In game, an LBA48 transfer of 65536 sectors or more no longer loops forever on 0-sector commands
  (hardening, no known trigger; same fix as ps2sdk f30f05eb). (e09f5482)
- GUI: The greeting screen shows a YAOPL logo, drawn in the style of OPL's logo by Berion, instead of OPL's.
  (fca82aea)
- Translations: Russian is translated completely, YAOPL's MMCE and UDPBD strings included, and 18 older
  Russian strings that said something else than the English or had typos are corrected. (yaopl-lang c459ea4,
  2084550)
- Translations: The Persian message about a fragmented VMC no longer crashes the menu: it read the slot number
  as text. The Cebuano VMC messages and auto-start countdown put a space before the name and the number
  instead of gluing them to the word before. (yaopl-lang 55d1ec2, 7fbdcb9)
- Translations: The documentation has a [Translations](https://yaopl.github.io/yaopl/translations/) page with
  how complete each language is. (ecbefe2c)
- Build: The toolchain image (ps2sdk, gsKit, ps2sdk-ports) is pinned by digest, so the same commit always
  embeds the same SDK modules; the release notes name the ps2sdk commit. A weekly canary build warns when the
  newest image differs. (78b8427d, 756f0141)
- Build: This changelog is kept again, with the upstream changes since 0.9.3 filled in; it replaces the
  per-commit `DETAILED_CHANGELOG`.
- Build: The 7z packages hold only the ELFs with `LICENSE` and `CREDITS`; `README.md` and the changelog are no
  longer packed, as the release notes list the changes. (b5980d64)
- Build: The release notes list the changelog entries added since the previous published build, with a link to
  the full file.
- Build: Problems with YAOPL builds can be reported in this repository's issues: the issue form asks for the
  `-yaopl` version, the device (now with UDPBD and MMCE) and the game ID. Problems with official OPL builds go
  to upstream. (f7b59653)
- Build: The README presents YAOPL, and a new user guide, `docs/USAGE.md`, replaces upstream's documentation:
  checked against the code, with UDPBD, MMCE and the ZSO limits. FORK.md was folded into this changelog and
  `CONTRIBUTING.md`. (91fb193c, 3980a6d4)
- Build: The user documentation is a site now, <https://yaopl.github.io/yaopl/>, with a page per topic:
  installing and launching, every menu and setting with its default, each device (USB, MX4SIO, iLink, the HDD
  as exFAT or APA, SMB with PC setup, UDPBD, MMCE), adding games, game settings, cheats, VMCs, in-game reset,
  NBD and troubleshooting, all checked against the code. It replaces `docs/USAGE.md`. (2e28eebd..30d34adc)
- Build: YAOPL moved to the `yaopl` organization on GitHub, <https://github.com/yaopl/yaopl>, and the
  documentation site to <https://yaopl.github.io/yaopl/>. Old repository, release and download links lead to
  the new place; the old site address does not.
- Build: The language package is built from YAOPL's own translation repository,
  <https://github.com/yaopl/yaopl-lang>, a fork of OPL's: it has all of OPL's translations and lists YAOPL's
  new strings (MMCE, UDPBD) for translators. Strings not translated yet are still shown in English.
- Build: The language package has Vietnamese: the packing script always left out the last language of the
  list, in upstream's packages too (32 of 33 languages).
- Build: YAOPL numbers its own versions: a release is `v2.0.0-yaopl` (tag `yaopl-v2.0.0`), and `latest` is a
  nightly, `v2.0.0-nightly.N-<commit>-yaopl`, the Nth change on `master` since the last release. Builds were
  called `v1.2.0-Beta-N-<commit>-yaopl` before, with N counting all commits, upstream's too. Releases are
  published from the tags, with the changes since the previous release in their notes. (b764a48e, cc70a691)
- Cheats: Cheat names in Cyrillic, Japanese and other non-ASCII scripts are kept: such a name was dropped and
  its codes joined the cheat before it, often the always-on Mastercode. (23a94a8c)
- MMCE: MMCE commands (game ID, IGR card switch) are sent only while the MMCE device is on (Auto or Manual),
  so a console without an MMCE card gets none. (b9e15b3a)
- MMCE: New device MMCE (Settings → MMCE Settings): games in `CD` and `DVD` on an MMCE memory-card emulator
  (SD2PSX, MemCard PRO 2, PSXMemCard Gen2) are listed and started, the ISO streamed in game by `mmcedrv` over
  SIO2. Modules built from `ps2-mmce/mmceman`. Not yet: VMCs on the card, games split into parts. (375bb693,
  fda97f71, 41819725, 1046d006)
- MMCE: For a game from any device the card gets the game ID and switches to the game's own card ("Send the
  game ID", on by default), after the last memory-card write. (243ed977)
- MMCE: IGR switches the cards that answered a ping back to their boot card (IGR slot "Auto"). (33a2b19a)
- MMCE: `make MMCE_NO_SDK_SIO2MAN=1` builds a comparison variant (`-nosio2man`) in which MMCE games keep their
  own SIO2MAN. (27bdbdeb)
- USB: `usbd_mini` is built from the ps2sdk USBD sources from before its rewrite (upstream Beta-2127): with
  the rewritten driver, Crash Bandicoot: The Wrath of Cortex took minutes to load from USB and then
  black-screened (upstream issue #1751). (09179482)
- In-game: IGR is sampled from the VBLANK_END interrupt handler again, not by a polling thread that busy games
  starve (upstream issue #1787, revert of #1762); Fatal Fury: Battle Archives Vol. 2 needs Mode 6 as before.
  (b8974f50)
- In-game: 2340-byte sector reads no longer write past the game's buffer. (8116fe66)
- Cheats: Picks of cheats that share a name are no longer lost on a reload. (fd642dc6)
- Cheats: A code line with address 0 (the second half of a multi-line code) no longer ends the cheat (upstream
  issue #1548). (e57decce)
- Cheats: More than 5 hooks or 250 codes are dropped instead of written past the cheat engine's tables.
  (6b645a00)
- Settings and config files: A CFG, theme or language file whose last line is a `#` comment without LF no
  longer crashes OPL. (5192529d)
- Settings and config files: A read error (device removed) ends the file instead of corrupting memory.
  (8dfdd548)
- Settings and config files: Keys of 32+ and values of 256+ characters are cut and terminated; a long line
  without `=` no longer overflows the prefix buffer. (38ae4e69, 4adde8b5)
- Settings and config files: A theme colour with more than three bytes (`#RRGGBBAA`) no longer writes past the
  colour. (51b45e09)
- Settings and config files: An `IPCONFIG.DAT` of 256 bytes or more no longer overflows the stack. (e8da9c86)
- Settings and config files: A failed settings save (e.g. full memory card) is reported instead of "Settings
  saved". (86ca55ac)
- Settings and config files: Looking for a USB device to save to no longer empties its `conf_opl.cfg`
  (upstream issue #1397). (fa5d4e56)
- Settings and config files: The default BGM path is no longer used as a format string. (0d28c4c5)
- VMC: Leaving the VMC menu no longer replaces a card of another size (4 MB, 128 MB, PCSX2 `.ps2`); Modify
  asks first. (bc35dcc8)
- VMC: VMC slots are found again after an aborted launch, so a game cannot use the previous game's VMC.
  (65765cfe)
- VMC: A failed cluster write while creating a VMC no longer makes the next creation hang. (885c201f)
- BDM: All parts of a split (UL) game must fit the fragment table together. (a5cbdb06)
- BDM: One VMC error is no longer reported again for the other slot. (1bf79385)
- BDM: Returning from the NBD server no longer crashes. (340e6338)
- HDD: The PS2 logo check reads 48 sectors, not 49. (9c5ed16c)
- ZSO: Images with a block size other than 2048 (maxcso's default for 2 GiB+ images is 16 KiB) are rejected
  instead of read as the wrong sectors. (9ad4afeb)
- ZSO: A corrupt block index gives a read error instead of a write before the buffer. (147e2493)
- ZSO: Reads of more than 256 sectors go in slices instead of running past the index cache. (4baf5921)
- GUI: Long text no longer makes `fntFitString()` scan and write past the string, and wrapped text no longer
  puts line breaks into the game's CFG (upstream issue #1713). (a4c0830f, 0f16d591)
- GUI: `GameCountText` gets the 60 bytes it formats into, not 1. (d784657c)
- GUI: A theme image file that is missing disables the element instead of drawing and freeing a garbage
  texture. (1183f4e8)
- GUI: A theme without an `ItemsList` keeps the default list instead of crashing on the first scroll.
  (669e2f8e)
- GUI: Grey and grey+alpha PNGs (art, themes) are loaded. (7a37c1ca)
- GUI: The scroll speed is restored after dialogs such as the VMC menu (upstream #1757). (33f7c226)
- GUI: A held button keeps repeating across the 29 s wrap of the tick counter (upstream #1757). (8a98f0fd)
- GUI: A `.lng` file name shorter than `lang_X.lng` (for example `ru.lng`) no longer crashes OPL at boot.
  (02ab252a)
- Launch and GSM: A boot file name over 16 characters (`$AltStartup`) no longer overflows the OSD history
  buffer. (80c5558d)
- Launch and GSM: A `$GSMVMode` outside the table of modes falls back to mode 0. (9058044e)
- Launch and GSM: Small hardening without a known trigger: a format string, terminators, the IOP patch mode
  compare. (d5a1237e)
- Build: The fork is named YAOPL; its builds carry `-yaopl` in the version string. (f849239b)
- Build: Every push to `master` publishes the `latest` pre-release with `SHA256SUMS`; only the build of the
  tip of `master` is published. (d38b20d6, cc7905ee)
- Build: Host checks (MSVC with AddressSanitizer) run on Windows for every push. (92d66096)
- Cheats: A comment line with leading blanks no longer becomes a cheat with an empty name. (c755d448)
- Cheats: The 251st cheat is no longer written past the cheat table. (44a003bd)
- Cheats: `make_code()` no longer writes one byte past its buffer for every code. (cdf07ba8)
- Cheats: A last line without LF is no longer read past the end of the buffer. (eeb57687)
- ISO and game list: A directory record with an empty name no longer hangs the game list or the game.
  (ff498c19)
- ISO and game list: The record length is read as one byte, so an extended attribute length no longer derails
  the scan. (af5b52b7)
- ISO and game list: File name lookups no longer copy from address 1 or past the 16-byte name, in the list and
  in game. (bfe726f5)
- ISO and game list: In-game lookups no longer overflow the current directory buffer on names longer than 14
  characters. (a2ff76fc)
- ISO and game list: `isValidIsoName()` no longer reads before short names such as `..`. (3722eb41)
- ISO and game list: A ZSO header with `align` above 11 is rejected instead of dividing by zero. (0253f98b)
- ISO and game list: `SYSTEM.CNF`: a `BOOT2` line without `=` no longer crashes. (43781768)
- ul.cfg and games.bin: A partial record at the end of `ul.cfg` is ignored instead of written past the game
  list. (2a57ca88)
- ul.cfg and games.bin: `ul.cfg` names are hashed as unsigned bytes, as PC tools do, also in `iso2opl` and
  `opl2iso`. Chunks of a game with a non-ASCII title that upstream OPL named itself may need renaming.
  (f244afff, da9af708)
- ul.cfg and games.bin: Each game listed in `games.bin` is looked up in the whole cache, without reading past
  it. (578ba6b5)
## OPL v1.2.0-Beta, 2025–2026 (upstream Beta-2197 – Beta-2271)

YAOPL is based on Beta-2271 (b3a21997, 2026-10-12).

### BDM, USB, MX4SIO
- Auto-launch of BDM games works again. (#1443, Beta-2197)
- The highest UDMA mode for BDM HDD is UDMA4, for SATA and IDE-to-SD adapters that fail at faster modes.
  (#1490, Beta-2201)
- MX4SIO regression and a sector cache bug fixed. (#1561, Beta-2210)
- BDM HDD: the HDD modules are loaded when the BDM config is loaded or saved, OPL waits for the drive, and
  its detection is more robust. (#1688, #1697, Beta-2235, Beta-2240)
- ATAD checks that the drive is ready during init. (#1717, Beta-2244)
- ZSO games on MX4SIO no longer black-screen: unaligned BDM reads go through an aligned buffer. (#1783,
  Beta-2254)

### In-game
- The in-game drivers use ps2sdk's `sio2man` (for MX4SIO); several sio2man versions can be hooked. (#1785,
  Beta-2248; YAOPL loads it only where needed since 2026-10-28)
- IGR is polled by a thread instead of the VBLANK interrupt. (#1762, Beta-2250; reverted in YAOPL on
  2026-10-23)
- CDVDMAN uses the real media size for its out-of-bounds checks. (#1763, Beta-2252)

### Games
- Ratchet & Clank: Up Your Arsenal starts over SMB, and its Korean and Japanese releases are patched.
  (#1761, #1780, Beta-2256, Beta-2267)

### SMB
- SMB games no longer show "Size: 0 MiB". (#1682, Beta-2228)

### VMC
- VMC groups: games that can share saves (a built-in list of title IDs) get the same default VMC name;
  groups extended later. (#1547, #1572, #1594, #1624, Beta-2207)

### Cheats
- Per-game PS2RD cheat selections are saved. (#1748, Beta-2265)

### GUI and settings
- Per-game parental lock (`$ParentalLock`): with a parental lock password set, starting the game asks for
  it. (#1779, Beta-2260)
- "Remember last played game" fixed. (#1552, Beta-2208)
- Label and title typo fixes (PADEMACRO "LY"). (#1596, Beta-2217)
- All languages of the language repository are built. (#1774, Beta-2264)

### Other
- Releases include the standalone `OPNPS2LD.ELF`. (#1421, Beta-2199)
- `opl2iso` no longer uses a pointer out of scope. (#1683, Beta-2226)
- Build fixes for GCC 15, lwNBD update, README, issue templates and contributing guidelines. (#1532,
  #1536, #1619, #1671, #1584, #1597, #1680, #1704, #1677, #1678, b7075ffb..b3a21997)

## OPL v1.2.0-Beta, 2024 (upstream Beta-2068 – Beta-2194)

### BDM, USB, HDD
- Several "mass" devices at once, BDM hot-plugging with connect and disconnect sounds, the theme element
  `BdmIndex` that shows which device is selected; menu and game settings fixes. (#1287, Beta-2109)
- Auto-launch of BDM games with arguments fixed after the BDM rework (mass0 only); all BDM modes are
  initialised and shut down properly. (#1333, Beta-2133)
- Internal HDD through BDM ("BDM HDD", exFAT), from grimdoomer's fork; only one HDD mode can be on at a
  time. (#1365, Beta-2194)
- An overflow in `bdmsupport.c` fixed. (#1315, Beta-2125)

### In-game and games
- Kidou Senshi Gundam: Ichinen Sensou fixed. (#1189, Beta-2072)
- The PS2 logo no longer crashes on some consoles; the IGR exit path can be 256 characters. (#1194,
  Beta-2077)
- Virtual device changed from `img` to `host`, with a proper close (after reverting the removal of UDNL).
  (#1307, Beta-2117)

### ZSO
- Out-of-bounds checks in the ZSO reader; updated Python ZSO script. (#1209, Beta-2082)

### Controllers
- The left analog stick navigates the GUI, with X and Y sensitivity settings. (#1303, Beta-2129)

### Cheats
- Cheats can be picked at launch. (#1364, Beta-2177)

### Video
- NTSC 240p and PAL 288p video modes. (#1341, Beta-2148)
- Holding Triangle + Cross at boot resets OPL's video mode to Auto. (#1332, Beta-2127)

### Apps
- Apps: media and format values in `title.cfg` are enforced; attribute images in subfolders are found.
  (#1241, Beta-2091)
- Apps with a `title.cfg` are found on memory cards (folders with "_" in the name); legacy app paths on
  `mass?` find their unit; themes can define app page elements. (#1289, Beta-2113)
- Apps can be the default start page; argument (`argv1=`) support fixed; legacy apps without a device
  number find it. (#1315, #1338, #1428, Beta-2125, Beta-2135, Beta-2184)

### GUI and themes
- Default theme: app name at the top of the app menu, APP and ZSO badges, HDD BDM icon; disc icons default
  to 64×64. (#1349, Beta-2139)
- Optional game count text. (#1362, Beta-2173)
- Per-font sizes for themes (`default_font_size=`). (#1393, Beta-2182)
- Faster boot: ISO sizes are read when the list is built, and the ISO scan is faster. (#1423, Beta-2189)
- Faster PNG loading. (#1357, Beta-2154)
- BMP and JPG images are no longer supported. (#1359, Beta-2169)
- The art cache is reset on a theme change; font bug after a gsKit update fixed. (#1406, #1431, Beta-2179,
  Beta-2186)
- Locking against race conditions in the GUI. (#1035, Beta-2081)
- OSD settings are saved correctly; PADEMU is no longer on by default in the global config. (#1245,
  Beta-2096)
- New `icon.sys`. (#1298, Beta-2119)

### Other
- Debug builds: PowerPC UART output, IOP free RAM report, documented debug colours. (#1269, #1344, #1348)
- GCC 14 fixes, bin2c, POSIX functions, `EE_SIO` from libkernel, CI actions v4, NBD documentation. (#1240,
  #1173, #1149, #1270, #1276, #1262, #1101, #1367, #1345)

## OPL v1.2.0-Beta, 2023 (upstream Beta-1965 – Beta-2065)

### In-game and games
- CDVDMAN rework by krat0s: file search sector bug fixed (#958); XPARAM patches from the boot ROM modules
  (#952); Eutechnyx patches for Big Mutha Truckers 2, Ford vs. Chevy, Ford Mustang: The Legend Lives and MTV
  Pimp My Ride PAL (#955); out-of-bounds seeks, `sceCdGetToc` and `sceCdTrayReq` (#961, #1000); demo
  `SYSTEM.CNF` `BOOT2` entries parsed (#960); custom sector sizes and semaphore fixes (#1051, #1052); CD TOC
  and DVD-DL TOC fixes (#1055). (Beta-1991 – Beta-2048)
- Patches are no longer applied to the wrong ELF files. (#1031, Beta-2022)
- IGR RPC data aligned. (#1036, Beta-2024)
- `PFS.IRX` uses the same parameters as wLaunchELF. (#1086, Beta-2049)

### BDM, USB, SMB
- USB and SMB accept longer file names, such as redump names. (0d6f8844, Beta-2017)
- BDM LBA ioctl updated for ps2sdk changes. (#927, Beta-1987)
- Faster directory reading. (#1108, #1115, #1116, #1125, #1126, Beta-2057)

### Controllers
- PADEMU supports PS2 guitars and PS3 Rock Band guitars. (c708f2d0, 20628ba6, Beta-1974 – Beta-1979)

### Video
- GSM H-position misalignment fixed. (#970, Beta-2012)
- OSD settings: screen aspect ratio and video mode. (#1057, Beta-2046)

### GUI
- Art PNGs load 2.5–3× faster. (#996, Beta-2016)
- Holding START at boot skips loading the config. (79f4f4fd, Beta-2020)
- Fixes: CNF files that caused infinite loops, NULL dereferences, division by zero in the FPS counter,
  renaming a file to the same name, a transition started during another. (#1037, #1038, #1039, #1040,
  #1041, Beta-2026 – Beta-2034)
- Apps are started with the "nocolour" ELF loader. (#839, Beta-1968)

### Other
- lwNBD from its official repository. (#887, Beta-1972)
- `EE_SIO` debug, IRX loading messages, IOP alignment fixes, linking changes, LTO removed, ps2homebrew
  Docker image. (743b6c53, #923, #864, #1131, #1137, c276b438)

## OPL v1.2.0-Beta, 2021–2022 (upstream Beta-1792 – Beta-1964)

### Formats
- ZSO compressed images (with a sector cache and a ZISO compressor). (c34a072b, Beta-1871; cleanups #707,
  #708)

### BDM, USB, MX4SIO
- FAT32 and exFAT games may be fragmented, up to 64 fragments like HDD; 2 KiB of IOP RAM freed. (#673,
  Beta-1895)
- The in-game USB driver is `usbd_mini.irx`. (#694, Beta-1901)
- BDM uses `fatfs` instead of `vfat`. (#660, Beta-1880)
- MCEMU (VMC) made compatible with MX4SIO. (#721, Beta-1917)
- Launch arguments auto-launch BDM games. (#852, Beta-1959)

### In-game
- Mode 1 (accurate reads) limits reads to the speed of a real drive: 900 KB/s CD, 2200 KB/s DVD. (#691,
  Beta-1899)
- PADEMU works over SMB and HDD with games that use USBD. (#659, Beta-1878)
- The console region is read at auto-launch, for the PS2 logo. (#581, Beta-1817)

### Controllers
- Pad macros change pad behaviour; DS3 and DS4 code unified. (#606, Beta-1836)

### Cheats
- PS2RD hook codes are detected. (#712, Beta-1910)

### SMB
- The SMB port can be any value up to 65353. (#552, Beta-1870)

### GUI and apps
- Theme background music in Ogg Vorbis; playback is muted during some operations to avoid stutter. (#836,
  Beta-1954)
- OSD language setting. (#724, #730, Beta-1920, Beta-1925)
- Apps take one argument (`argv1=` in `title.cfg`). (#688, Beta-1904)
- Apps are started with ps2sdk's elf-loader. (#564, Beta-1826)
- No OSD history is written to PS1 cards, and only when a usable PS2 card is found. (#579, #838, Beta-1819,
  Beta-1955)

### Languages
- Languages and fonts moved to the Open-PS2-Loader-lang repository and a separate language pack;
  GUI strings are generated. (#650, #609, #612, Beta-1874)
- Translations: Albanian (new), Bulgarian, Cebuano, Greek, Polish, Turkish (with font). (#565, #629, #547,
  #549, #585, #567, #572, #554, #573, #633)

### Other
- lwNBD protocol improvements. (#546, Beta-1795)
- Test ISO (`opltestiso`) for OPL's in-game specs; warning fixes and cleanups; CI fixes and MEGA backups.
  (#684, #718, #720, #807, #619, #743, #744, #856, #802, #607, #639, #717)

## OPL v1.1.0 (rev1791, 2021-09-09)

Upstream's summary ("New in OPL v1.1.0"):

- GCC 11 toolchain for both EE and IOP.
- NBD server for transferring files over the network.
- New USB mass storage driver (BDM).
- i.LINK (IEEE1394, FireWire) support (experimental).
- MX4SIO support (experimental).
- Internal HDD users can change the partition that stores settings.
- HDD OSD users can use OPL with OPL Launcher.
- GUI settings reorganised and renamed.
- Known regression: movie playback hangs on HDD after the new IOP toolchain (upstream issue #466).

The changes since v1.0.0 (rev1590 – rev1791):

### BDM, USB, MX4SIO, i.LINK
- USB driver replaced by the block device manager (BDM), with i.LINK support. (#417)
- MX4SIO support; BDM device selection reworked in the GUI. (#467)
- Loading the block device modules other than USB is optional. (#444)
- BDM devices can be ejected and inserted in game. (#479)
- The device icon and text change when a new block device is detected. (#525, #422)
- The check for fragmented USB games is always done; its option was removed. (#378)
- Longer game names in the USB utility; up to 64 modules. (#484, #521)
- USBExtreme: invalid entries are ignored, the CRC32 is guarded against overflow, a USBExtreme issue fixed.
  (#485, #517, #531)

### HDD and NBD
- A partition other than `+OPL` can be used. (#423)
- Launch arguments from HDD OSD auto-boot games. (#481)
- The broken HDL server was replaced with an NBD server (lwNBD); `nbd_default_export` option. (#415, #506,
  #523, #530)

### In-game
- IGR can boot the exit ELF from USB; IGR power-off bug fixed; IGR stack 4 KiB. (#378, #486, #354)
- imgdrv finds its offsets instead of using constants; CDVDMAN and CDVDFSV split and cleaned up; SMB file
  name argument extended to 88 characters. (#443, #445, #433)

### GUI, apps, controllers
- Apps menu. (#349)
- IGS and PADEMU enabled by default. (#354)
- ELF loading and app support fixed. (#362)
- Controller Settings menu, with global PADEMU. (#459)
- Info page strings translated at init; JPEG via libjpg; better texture loading. (#393, #475, #478)
- Memory cards handled with POSIX functions, with fixes. (#461, #522, #535)
- Static IP and DHCP network settings fixed. (#502)
- New `icon.sys` flag. (#416)

### Languages and other
- Translations: Cebuano (new), Filipino, Polish, Portuguese (Brazil), Spanish and others. (#352, #361,
  #377, #381, #390, #391, #400, #424, #470, #489, #520, #536, 1cba3f96, 0265d2ac)
- GCC 10 and 11 fixes, module cleanups, clang-format, debug builds fixed, MEGA backups, issue forms,
  README. (#354, #364, #394, #433, #434, #436, #439, #441, #442, #449, #450, #451, #452, #458, #471, #480,
  #483, #493, #494, #515, #539)

## OPL v1.0.0 (rev1589, 2021-01-06)

From upstream's "Simplified Changelog" in the v1.0.0 release notes (changes since 0.9.3):

### Core
- Game list cache (HDD, SMB, USB) speeds up scanning; on HDD it is always used unless the list is refreshed
  or a game is renamed or deleted.
- New app system: apps in their own folders under `APPS` with a `title.cfg`; the old system still works.
- GSM, cheats, VMC and childproof mode are always built in.
- PADEMU: DualShock 3 and 4 over USB and Bluetooth.
- Languages from all devices (USB, HDD, SMB) in an `LNG` folder.
- Config files: memory cards first, then the boot device, then all supported devices, else memory card.
- newlib 1.14.0; new toolchain fixes.
- GSM, IGR and SMB (SMSTCPIP): many fixes and improvements.
- IGS: in-game screenshots (not functional).
- Skip videos also skips `.BIK` and `sceMpegIsEnd` based videos.
- The PS2 logo is shown when a game starts (when disc and console regions match).
- Mode 7 (high module storage) removed.
- Updated HDD and PFS drivers; ps2sdk's DEV9, ATAD, HDPro ATAD and USBHDFSD used directly; updated in-game
  DEV9, SMAP, ATAD and HDPro ATAD.
- CDVDFSV fixes; better USB device detection; in-game USBD priority 2 and 3; faster USB transfers.
- Display X and Y offset settings; BMP textures.
- The power button shuts the PS2 down in the GUI while the network adapter is in use; USB devices and the
  HDD are shut down before power-off.
- In-game SMAP: workaround for the MAL Rx FIFO hang, DMA block size 64.
- `+OPL` partition creation fixed; HDLoader game size fixed; reads across two parts fixed.
- EE core free memory starts at 0x00084000, for more memory.
- lwIP 2.0; DNS changed only for static IP; DHCP defaults.
- HDPro communication fixed.
- HDLDump server crash and freeze fixed.
- HDD size calculation, power-off processing, ARP PBUF allocation fixed.
- Renaming on USB disks fixed; memory card icon fixed; PS3 backwards compatibility (SMB) fixed.
- Per-game cheat engine fixed on all devices; missing font when changing language fixed.

### GUI
- New default theme and logo by Berion.
- Game menu with global game settings (`conf_game.cfg`) for GSM, PS2RD and PADEMU; per-game settings take
  priority. Delete and rename moved into the game menu.
- Information page on the Square button, in the default theme too.
- High-resolution video modes, with a confirmation when the video mode changes.
- Sound effects for the intro and menus.
- New Start and Select buttons; fade transition; notifications; updated About page.
- Font POEVeticaNew; font scaling for non-square pixels and the auto video mode; vertical alignment fixed;
  the internal font is reloaded when switching back to internal English.
- 4- and 8-bit PNGs; interlaced frame mode and 16-bit alpha blending fixed; scaled button icons.
- Button to reset the theme to the built-in colours.
- Auto or manual refresh of the apps list.
- Themes: textures up to 1024×1024; translated info page keys; overlays and several default textures in the
  internal theme; centred `ItemsList`; `aligned=1` for hints; `AttributeText` and `AttributeImage` on the
  main page; MiB suffix for `#Size`; fixes for info page elements, fonts and loading from the "manual"
  device.
- VMC name shown when first entering the menu.

### Game fixes
- Kingdom Hearts II (Gummi missions), SOS: The Final Escape / Disaster Report, Project Zero / Fatal Frame,
  Samurai Warriors 2, Warriors Orochi series.
- GSM: God of War (SCUS-97399), Sega Genesis Collection (SLUS-21542).
- Patches: Virtua Quest, .hack (PAL), Ultimate Pro Pinball, Ferrari Challenge: Trofeo Pirelli, Shadow Man:
  2econd Coming, Ridge Racer V (USB), SRS: Street Racing Syndicate, The Fast and the Furious, Harvest Moon:
  A Wonderful Life, Ratchet & Clank 3 (HDD), Shaun Palmer's Pro Snowboarder.
- Fixes: Twisted Metal: Black Online, State of Emergency (HDD), Metal Gear Solid 3: Subsistence, Splinter
  Cell: Double Agent, Splinter Cell (USB), Baldur's Gate: Dark Alliance 2 (USB), God of War II (USBExtreme,
  SMB), Legacy of Kain: Soul Reaver 2 (USBExtreme).

## Before v1.0.0

The entries below are the original `CHANGELOG` of OPL, last written for 0.9.3 (2015), converted to Markdown.

### OPL 0.9.3
#### Core

- New RDB-based DECI2 Debugging System.
- Refactored and optimized core modules.
- New CDVDMAN streaming mechanism for better performance.
- Kinder, gentler, and hopefully better IGR mechanism
- Added support for the DTL-T10000, for debugging.
- Updated USB drivers - support drives greater than 1TB but less than 2TB in capacity, plus better reliability.
- Eliminated the need for user-configurable delays in USB support.
- Improved performance and stability of SMB support.
- VMC Fix - Slot2 should no longer clone Slot1 and vice versa
- Removed obsolete Compatibility Modes and needless GSM video modes.
- New PS2RD Cheat Engine port by Doctorxyz.
- Per-Game GSM Support.
- New super-slim EE core, making mode 1 (Alt EE core) obsolete.
- Added Accurate Reads mode, for emulating the reading speed and behaviour of the CD/DVD drive.
- The CallBack Timer (CBT) setting now uses a standardized value and was merged into Accurate Reads mode.
- Added a workaround for some clone network adaptors.
- Mode 7 "IOP threading hack" removed, as the new streaming mechanism has taken care of all related issues.
- New compatibility mode: "high module storage". For games that need OPL to store its modules elsewhere, to avoid a memory conflict.
- Improved reading performance of the cdrom device.
- Improved reliability of some CDVDMAN functions, for more consistent behaviour.
- Improved DVD-DL support.
- Changed "Disable DVD-DL" to "Emulate DVD-DL" to better explain what it does. It's to emulate DVD-DL support, for DVD9 rips (games with their 2 layers compacted into 1).
- Fixed support for SMB usernames and passwords that are longer than 16 characters (limit extended to 31).
- Fixed renaming functionality for USBExtreme games.
- Various fixes for games.

#### GUI

- Improved the behaviour of the auto-refresh option.
- GSM moved to Game Options menu since its a Per-Game option now.
- Infamous "Blockhead Grande" theme bug SQUARSHED! (Themes should now get loaded correctly, regardless of display settings).
- New logo design by Berion, new changed icons by El_Patas.
- Better error reporting and help messages in general.
- Network settings are now saved in opl_network.cfg.
- Improved stability, design and UI responsiveness.
- Added missing icon hints.
- Fixed the problem with the HDLDump server being difficult to shut down properly.
- Fixed HDD corruption caused by deleting a game from the HDD unit.
- Added a new network update mechanism, which allows game compatibility records to be automatically downloaded from the OPL-CL service.
- SMB server can now be specified by its NetBIOS name.
- IP address configuration can be set automatically with DHCP, although it is still recommended to reserve an IP address for the PS2 in the network because the DHCP reservation will not be ever renewed while in-game.
- Renamed "Enable Delete and Rename" to "Enable Write Operations".
- Changed the delete and modify VMC controls to be disabled instead of hidden, when writes are disabled.
- ISO disc images no longer strictly need to be named in a special format. Simply put your ISO files in your CD and DVD folders and OPL will find them.
- Renamed compatibility modes to have actual names.
- Game history will now be updated; the "towers" behind the "SONY Computer Entertainment" boot screen should continue to grow.
- Added support for OTF fonts.
- Added support for fonts stored on the HDD unit (root of +OPL partition; hdd0:+OPL/) and USB device (root of device; mass0:/).

#### Game fixes and compatibility
- New patch: Choujikuu Yousai Macross
- New patch: Super Robot Wars IMPACT
- Drive state of sceCdStandby has been changed to PAUSE: Check iTV
- New patch: Ratchet and Clank: Up Your Arsenal
- New patch: The Oneechanbara
- New patch: The Oneechanpuruu
- New streaming mechanism for better playability: Various BEMANI games
- Initialization is performed for search functions: Lifeline

### OPL 0.9.2
(rev672)

#### Core

- Merged OPL/OPLGSM codebase
- GSM Core 0.36.R - don't worry; 0.38 will be coming back better than ever!
- GSM Core no longer supports 576p, 1080p, Skip FMVs - this is temporary.
- GSM Core is now IGR and CodeBreaker/PS2rd compatible!
- GSM no longer included in Childproof builds.
- Unified Makefile builds from single source tree via "GSM=0/1" argument (ala VMC/No VMC)
- The integrated LibFreeType was removed, upgraded, and moved into the PS2SDK
- Upgraded LIBPNG, ATAD, HDPro, and DEV9 Module Code
- Added code for the power-off RPC, for older games. They should be compatible now.
- Added power-off processing code. Games should be able to power-off the console now.
- Optimzed CDVDMAN and CDVDFSV
- Support for PSX (DVR) units
- Built-in TLB reinitialization mechanism - no more EECORE hooking and patching.
- Added compatibility mode 7 - "IOP Threading Hack".
- Replaced SMAP driver.
- Modified SMSTCPIP.
- Ported a version of ETH Mode types from doctorxyz's OPLGSM.
- Retired the built-in kernel update for Protokernel consoles (SCPH-10000 and SCPH-15000).

#### GUI

- New "Show GSM on Main Menu" toggle in "Settings" (GSM Builds Only)
- New "GSM Settings" on Main Menu - easier navigation! (GSM Builds Only)
- Fixed issue with HDLSERV and GSM Settings not showing up immediately.
- Full Ethernet controls - can disable Auto-Negotiation and force a mode.
- Some things were are worded differently as less words carry more freight.
- Removed redundant UDMA modes - PS2 will never handle UDMA 5 and UDMA 6.
- Updated SMB support to give better error messages.
- Network error codes changed slightly - 300 for connection errors, 301 for logon errors, 302 for ECHO isalive
  test failing, and 303 for share opening errors.

#### Compatibility

- Kingdom Hearts 2 Final Mix (Gummi Fix)
- Baldar's Gate Series (HDD)
- Champions of Norrath (HDD)
- New Mode 7 fixes stuttering with games such as DDR and Pop'N Music (HDD ONLY!)

### OPL 0.9

#### Core
- various fixes
- added configurable USB delay (increase this value to avoid stucking on yellow or orange screen when using USB mode)
- added delay to CDVDMAN functions (initially for Shadow Heart 2)
- compatibility mode 7 which is unused now (remove useless code)
- update to comply with the latest PS2SDK
- added better cd tray emulation (fixes A2M games)
- fix for DVD9 games
- added HD Pro support

#### GUI
- various fixes
- Disabled auto-refresh of game list by default
- layout adjustment
- added hidden options to be able to define your own device delay before loading ART/config files
- added skinnable start, select and action buttons
- language files update
- added "list share" feature: select one share from the list and connect it to display the games
- added ETH Prefix
- Support for Right-to-Left scripting (Hebrew, Arabic, Syriac, Tifinar, ... languages)
- automatically create the CFG folder
- Configuration related to DNAS/VMC/DMA/COMPAT are now saved in single per game files on each media
- added a delay before loading per-game config file (same as Art, to not spam when navigating very fast)
- hints now display either "Info" or "Run"
- added loading attributes from a per-game file
- added information page
- Mode setting (PAL/NTSC/Auto)
- Longer first-press delay for pads - more comfy controls on max repeat speed
- short ELF name in App list (looks better for most themes)
- Added option to specify the HDD spin down time out
- Added theme support for multiple fonts
- configurable IGR path (limited to 32 characters)
- switch default background mode to "Art"

### OPL 0.8
#### Core
- secure authentication for the SMB module (to enable security and write access)
- various SMB tweaks (NBSS keep-alive, small ingame speed-up)
- improved NAS support for more devices (WD MyBook, Digitus NAS, ...)
- added an embedded HDLServer into OPL to install HDD games
- various debugging output option (to help debug failing games)
- add compatibility for new external USB HDD with 1k/2k/4k cluster file
- add full compatibility for old PS2 consoles (10k JAP)
- ISO format added for USB too (still there is the 4Gb file limit, which is not present on SMB)
- new compatibility modes
- childproof mode (settings are locked)
- alternate startup mode, for loading/saving configuration files from USB and HDD (no need for MC in this case)
- added new menu to launch external ELF applications
- added VMC (Virtual Memory Card) support
- improved general compatibility (many specific games now working for the first time)

#### GUI
- new multi-threaded and redesigned GUI
- support for Cover, Icon and Background Art
- skinnable interface using theme
- added "rename" and "delete" functionalities for games lists

### OPL 0.7
- Improved network startups and various fixes
- Added internal HDD support (HDL format)
- Added the ability to choose the default menu (USB, NET, HDD games)
- Added the ability to use a valid disc ID for online games
- Added a fix for modchip freezing (maybe not all modchips)
- Many new game compatibility fixes
- Added IGR (In Game Reset) with a disable per game option
- Added Alphabetical Sorting
- Changed Settings categories
- Added page scrolling (R1/L1)
- Added support for user language files
- Network support added for PS3
- Zipped Theme support
- Added PCMCIA support for the 18K

### OPL 0.6
- Switched to whole new IOP core
- Changed some compatibility modes effect
- Added compatibility for DVD9 layer1
- Improved overall speed for both SMB/USB
- Support for more embedded NAS devices
- Support for multiple partitions

### OPL 0.5
- Added compatibility modes
- isofs/cdvdman now use disc type
- cdvdman now has streams
- Added network support
- Static interface available
- Added language support
- It works in PS3 with PS2 emulation by software

### OPL 0.41
- Added theme support
- Now uses latest usb driver from svn

### OPL 0.4
- New interface
- Core and drivers 100% open source

### OPL 0.3
- Discontinued (open usb loader starts)

### OPL 0.2b
- It uses a embedded free USBD module.
- Select video mode

### OPL 0.2
- Replace USB Advance's reset function for a reset using "rom0:UDNL rom0:EELOADCNF" as argument.
- It works in PS3 with PS2 emulation by software

### OPL 0.1
- Replaces ExtraeModulo() function instead of run after that.

### OPL 0.0
- Initial version
