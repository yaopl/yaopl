# Troubleshooting

## YAOPL itself

**YAOPL freezes on the logo or a grey screen.**
Settings from another OPL build may be the cause. Hold START while YAOPL starts to skip loading them, then set
it up and Save Changes again.

**Nothing on screen after start.**
The menu's video mode may not suit the TV. Hold Triangle + Cross while YAOPL starts to reset it to Auto.

**The game list is empty, or only the main menu shows.**
No device is turned on yet: see [First launch](getting-started.md#first-launch). A device set to Manual starts
when you press confirm on its page. On a USB drive, check that the games are in `CD` or `DVD` (or in the folder
set as *BDM Prefix Path*).

**A game is missing from the list.**
Check the file extension (`.iso` or `.zso`), that the game is in `CD` or `DVD`, and that its title is at most 160
characters. A damaged image whose `SYSTEM.CNF` cannot be read is not listed. Press SELECT to refresh. If you
replaced an image with another game under the same name, delete `games.bin` in that folder.

**Settings are not kept.**
Choose **Save Changes** in the main menu (or in the game menu for game settings). With *Notifications* on, YAOPL
says where it saved them; see [Where settings are saved](getting-started.md#where-settings-are-saved).

## Starting games

**A game does not start or freezes.**
Try the [compatibility modes](game-settings.md#compatibility-modes) one at a time with **Test Changes**. Check
whether official OPL behaves the same before you [report it](https://github.com/yaopl/yaopl/issues).

**A game freezes on a white screen.**
Its main executable could not be read: the image is fragmented or damaged.

**Error, the game is fragmented.**
The image is in more than 64 pieces on the drive. See
[Copy games so they are not fragmented](devices/usb.md#copy-games-so-they-are-not-fragmented).

**Error, could not run the item.**
The image could not be opened or is not valid; check the file on a PC.

**A ZSO game is refused.**
YAOPL reads ZSO files with 2048-byte blocks only. Recompress it: see [ZSO](games.md#zso).

**Videos stutter.**
The device is too slow for the game (USB 1.1, SMB). Use a faster device, or skip videos with
[Mode 4](game-settings.md#compatibility-modes).

**A VMC message at start.**
See [Virtual memory cards](features/vmc.md#notes).

**Error: failed to load cheat file.**
Cheats are on for the game but `CHT/<game ID>.cht` is missing or cannot be read. Put the file there or turn the
cheat engine off in Cheat Settings.

**IGR goes to the console's browser instead of YAOPL.**
That is what IGR does when *IGR Path* is empty: set it to the path of `OPNPS2LD.ELF` or of your launcher. See
[Where IGR goes](features/igr.md#where-igr-goes).

## Network and devices

- SMB connection errors: see [SMB share → Errors](devices/smb.md#errors).
- A game over SMB freezes after the console was left idle: see
  [SMB share → A game freezes after an idle time](devices/smb.md#a-game-freezes-after-an-idle-time).
- Network adapter, link and DHCP errors: see [Network setup → Network errors](devices/network.md#network-errors).
- *The network adapter is used by UDPBD / by SMB*: only one of them can run; see [UDPBD → Limits](devices/udpbd.md#limits).
- `<n>: HardDisk Drive not detected.` or `<n>: HardDisk Drive not formatted.` on the APA HDD page: the drive or
  its adapter was not found, or the drive has no usable APA partitions. An exFAT drive belongs under
  *HDD (GPT/MBR)* instead. See [Internal HDD](devices/hdd.md).
- *No MMCE card answered.*: see [MMCE → Limits](devices/mmce.md#limits).

## Reporting a problem

[Open an issue](https://github.com/yaopl/yaopl/issues) with the YAOPL version (START → About), the device and
how it is formatted, the game and its ID, the compatibility modes you tried, and whether official OPL behaves the
same.
