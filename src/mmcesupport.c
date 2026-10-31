/*
  Copyright 2024-2025, qnox32 and BBsan (ps2-mmce/Open-PS2-Loader)
  Licenced under Academic Free License version 3.0
  Review Open PS2 Loader README & LICENSE files for further details.

  Games on an MMCE memory-card emulator (SD2PSX, MemCard PRO 2, PSXMemCard Gen2). The card's SD card is
  the iomanX file system "mmce" of mmceman.irx (mmce0: is memory-card slot 1, mmce1: slot 2) and holds
  the same CD, DVD, ART, CFG... folders as a USB device. In game the ISO is read through mmcedrv.irx, see
  modules/iopcore/cdvdman/device-mmce.c.
*/

#include "include/opl.h"
#include "include/lang.h"
#include "include/gui.h"
#include "include/supportbase.h"
#include "include/mmcesupport.h"
#include "include/util.h"
#include "include/themes.h"
#include "include/textures.h"
#include "include/ioman.h"
#include "include/system.h"
#include "include/extern_irx.h"
#include "include/cheatman.h"
#include "modules/iopcore/common/cdvd_config.h"

#include <ps2sdkapi.h>
#define NEWLIB_PORT_AWARE // for fileXioIoctl2 and fileXioDevctl; files are opened with open(), as in bdmsupport.c
#include <fileXio_rpc.h>
#include <delaythread.h>

// mmceman devctl and ioctl2 codes (ps2-mmce/mmceman: mmceman/include/mmce_cmds.h, mmceman/src/mmce_fs.c)
#define MMCE_DEVCTL_PING       0x1  // -1 when no card answers on that port, < 0 without mmceman
#define MMCE_DEVCTL_GET_STATUS 0x2  // bit 0: the card is busy
#define MMCE_DEVCTL_SET_GAMEID 0x8  // the game ID string, so the card switches to that game's memory card
#define MMCE_IOCTL2_GET_FD     0x80 // the card's own descriptor of an open file

static char mmcePrefix[40]; // "mmceN:/" plus the user's prefix: the folder that holds CD, DVD, ART, CFG...
static int mmcePort = -1;   // SIO2 port of the card the prefix points at (2 = slot 1, 3 = slot 2), -1 = none
static int mmceULSizePrev = -2;
static time_t mmceModifiedCDPrev;
static time_t mmceModifiedDVDPrev;
static int mmceGameCount = 0;
static base_game_info_t *mmceGames = NULL;
static int mmceModulesLoaded = 0;
static unsigned char mmceThemesLoaded = 0;    // the card's THM folder is listed
static unsigned char mmceLanguagesLoaded = 0; // and its LNG folder

// forward declaration
static item_list_t mmceGameList;

// The prefix for a port: the user's prefix is a folder on the card, without a leading slash and with a
// trailing one.
static void mmceBuildPrefix(int port)
{
    const char *folder = gMMCEPrefix;
    size_t len;

    while (*folder == '/')
        folder++;

    snprintf(mmcePrefix, sizeof(mmcePrefix), "mmce%d:/%s", (port == 3) ? 1 : 0, folder);
    len = strlen(mmcePrefix);
    if (*folder != '\0' && mmcePrefix[len - 1] != '/' && len < sizeof(mmcePrefix) - 1) {
        mmcePrefix[len] = '/';
        mmcePrefix[len + 1] = '\0';
    }
}

// The SIO2 ports that have an MMCE card, as bits 2 and 3 (port 2 = memory-card slot 1, port 3 = slot 2).
// Pinged once per session, or again when asked (mmceman must be loaded).
static int mmceProbePorts(int again)
{
    static int ports = -1;

    if (ports < 0 || again) {
        ports = 0;
        if (fileXioDevctl("mmce0:/", MMCE_DEVCTL_PING, NULL, 0, NULL, 0) >= 0)
            ports |= 1 << 2;
        if (fileXioDevctl("mmce1:/", MMCE_DEVCTL_PING, NULL, 0, NULL, 0) >= 0)
            ports |= 1 << 3;
    }

    return ports;
}

// The configured slot, or in Auto mode the first port that answers a ping. Returns the port, or -1.
static int mmceFindCard(int again)
{
    int ports;

    if (gMMCESlot == MMCE_SLOT_1)
        return 2;
    if (gMMCESlot == MMCE_SLOT_2)
        return 3;

    ports = mmceProbePorts(again);
    if (ports & (1 << 2))
        return 2;
    if (ports & (1 << 3))
        return 3;

    return -1;
}

// Keeps the prefix in step with the settings. The card is looked for again only when the settings
// changed or no card was found before, so the ports are not pinged on every refresh of the list.
static void mmceSetPrefix(int force)
{
    static int slotUsed = -1;
    static char folderUsed[sizeof(gMMCEPrefix)] = "";
    char oldPrefix[sizeof(mmcePrefix)];
    int port;

    if (!force && mmcePort > 0 && slotUsed == gMMCESlot && !strcmp(folderUsed, gMMCEPrefix))
        return;

    port = mmceFindCard(force || mmcePort < 0);
    slotUsed = gMMCESlot;
    strcpy(folderUsed, gMMCEPrefix);
    strcpy(oldPrefix, mmcePrefix);
    mmceBuildPrefix(port);
    mmcePort = port;

    // The slot or the folder changed: the themes and languages of the old folder go, the new one's are listed by
    // mmceNeedsUpdate()
    if (oldPrefix[0] != '\0' && strcmp(oldPrefix, mmcePrefix))
        sbDropThemesLanguages(oldPrefix, &mmceThemesLoaded, &mmceLanguagesLoaded);

    LOG("MMCESUPPORT prefix %s, port %d\n", mmcePrefix, mmcePort);
}

void mmceLoadModules(void)
{
    if (mmceModulesLoaded)
        return;

    LOG("[MMCEMAN]:\n");
    if (sysLoadModuleBuffer(&mmceman_irx, size_mmceman_irx, 0, NULL) == 0)
        mmceModulesLoaded = 1;
    else
        LOG("MMCESUPPORT mmceman did not load\n");
}

// The memory-card slots whose MMCE card IGR switches back to its boot card (bit 0 = slot 1, bit 1 = slot 2):
// the setting, or with "Auto" the slots where a card answered a ping. None while the MMCE device is off, so
// OPL does not send MMCE commands to the memory-card ports of a console without an MMCE card.
int mmceIGRSlots(void)
{
    int ports;

    if (gMMCEStartMode == START_MODE_DISABLED)
        return 0;

    if (gMMCEIGRSlot != MMCE_IGR_AUTO)
        return gMMCEIGRSlot & 3;

    mmceLoadModules();
    if (!mmceModulesLoaded)
        return 0;

    ports = mmceProbePorts(0);
    return ((ports >> 2) & 1) | (((ports >> 3) & 1) << 1);
}

// Tells the MMCE card(s) which game starts, so a card switches to that game's own memory card. It must run
// after the last write to the real memory card (last played, OSD history): from then on mc0: or mc1: is
// the game's card. The card is busy for a moment after the command: it reports busy while it loads the new
// card and does not answer at all while it swaps them (an SD2PSX is silent for 500 ms and more), so a status
// < 0 counts as busy, as in mmceigr. The wait is bounded like mmceigr's (7.5 s). Only while the MMCE device
// is on, as for mmceIGRSlots().
void mmceSendGameID(const char *bootpath)
{
    static const char *const devices[2] = {"mmce0:/", "mmce1:/"};
    char gameid[17];
    const char *name, *p;
    int ports, port, i, status;
    size_t len;

    if (!gMMCEEnableGameID || gMMCEStartMode == START_MODE_DISABLED || bootpath == NULL)
        return;

    // The boot file name alone, without the path and the version: SLUS_202.38.
    name = bootpath;
    for (p = bootpath; *p != '\0'; p++) {
        if (*p == '/' || *p == '\\' || *p == ':')
            name = p + 1;
    }
    for (len = 0; name[len] != '\0' && name[len] != ';' && len < sizeof(gameid) - 1; len++)
        gameid[len] = name[len];
    gameid[len] = '\0';
    if (len == 0)
        return;

    mmceLoadModules();
    if (!mmceModulesLoaded)
        return;

    ports = mmceProbePorts(0);
    for (port = 2; port <= 3; port++) {
        if (!(ports & (1 << port)))
            continue;

        LOG("MMCESUPPORT game ID %s to %s\n", gameid, devices[port - 2]);
        DelayThread(200 * 1000);
        if (fileXioDevctl(devices[port - 2], MMCE_DEVCTL_SET_GAMEID, gameid, len + 1, NULL, 0) < 0) {
            LOG("MMCESUPPORT the card did not take the game ID\n");
            continue;
        }
        for (i = 0; i < 75; i++) {
            DelayThread(100 * 1000);
            status = fileXioDevctl(devices[port - 2], MMCE_DEVCTL_GET_STATUS, NULL, 0, NULL, 0);
            if (status >= 0 && (status & 1) == 0)
                break;
        }
        LOG("MMCESUPPORT card status %d after %d x 100 ms\n", status, (i < 75) ? i + 1 : i);
    }
}

void mmceInit(item_list_t *itemList)
{
    LOG("MMCESUPPORT Init\n");
    mmceULSizePrev = -2;
    mmceModifiedCDPrev = 0;
    mmceModifiedDVDPrev = 0;
    mmceGameCount = 0;
    mmceGames = NULL;

    configGetInt(configGetByType(CONFIG_OPL), "usb_frames_delay", &mmceGameList.delay);
    mmceGameList.updateDelay = -1; // no periodic rescans: the card is not hot-plugged

    mmceLoadModules();
    mmceSetPrefix(1);

    mmceGameList.enabled = 1;
}

item_list_t *mmceGetObject(int initOnly)
{
    if (initOnly && !mmceGameList.enabled)
        return NULL;
    return &mmceGameList;
}

static int mmceNeedsUpdate(item_list_t *itemList)
{
    char path[256];
    int result = 0;
    struct stat st;

    mmceSetPrefix(0);

    if (mmceULSizePrev == -2)
        result = 1;

    sprintf(path, "%sCD", mmcePrefix);
    if (stat(path, &st) != 0)
        st.st_mtime = 0;

    if (mmceModifiedCDPrev != st.st_mtime) {
        mmceModifiedCDPrev = st.st_mtime;
        result = 1;
    }

    sprintf(path, "%sDVD", mmcePrefix);
    if (stat(path, &st) != 0)
        st.st_mtime = 0;

    if (mmceModifiedDVDPrev != st.st_mtime) {
        mmceModifiedDVDPrev = st.st_mtime;
        result = 1;
    }

    if (!sbIsSameSize(mmcePrefix, mmceULSizePrev))
        result = 1;

    // update Themes
    if (!mmceThemesLoaded) {
        sprintf(path, "%sTHM", mmcePrefix);
        if (thmAddElements(path, "/", 1) > 0)
            mmceThemesLoaded = 1;
    }

    // update Languages
    if (!mmceLanguagesLoaded) {
        sprintf(path, "%sLNG", mmcePrefix);
        if (lngAddLanguages(path, "/", mmceGameList.mode) > 0)
            mmceLanguagesLoaded = 1;
    }

    sbCreateFolders(mmcePrefix, 1);

    return result;
}

static int mmceUpdateGameList(item_list_t *itemList)
{
    sbReadList(&mmceGames, mmcePrefix, &mmceULSizePrev, &mmceGameCount);
    return mmceGameCount;
}

static int mmceGetGameCount(item_list_t *itemList)
{
    return mmceGameCount;
}

static void *mmceGetGame(item_list_t *itemList, int id)
{
    return (void *)&mmceGames[id];
}

static char *mmceGetGameName(item_list_t *itemList, int id)
{
    return mmceGames[id].name;
}

static int mmceGetGameNameLength(item_list_t *itemList, int id)
{
    return ((mmceGames[id].format != GAME_FORMAT_USBLD) ? ISO_GAME_NAME_MAX + 1 : UL_GAME_NAME_MAX + 1);
}

static char *mmceGetGameStartup(item_list_t *itemList, int id)
{
    return mmceGames[id].startup;
}

static void mmceDeleteGame(item_list_t *itemList, int id)
{
    sbDelete(&mmceGames, mmcePrefix, "/", mmceGameCount, id);
    mmceULSizePrev = -2;
}

static void mmceRenameGame(item_list_t *itemList, int id, char *newName)
{
    sbRename(&mmceGames, mmcePrefix, "/", mmceGameCount, id, newName);
    mmceULSizePrev = -2;
}

void mmceLaunchGame(item_list_t *itemList, int id, config_set_t *configSet)
{
    int fd, iso_fd, index, compatmask = 0;
    int EnablePS2Logo = 0;
    int result, vmc_id;
    char partname[256], filename[32], vmc_name[32];
    base_game_info_t *game;
    struct cdvdman_settings_mmce *settings;
    u32 layer1_start, layer1_offset;
    unsigned short int layer1_part;
    u64 isoTotalBytes;

    game = &mmceGames[id];

    // In game the image is read through one descriptor on the card: a game split into parts cannot be
    // streamed yet.
    if (game->format == GAME_FORMAT_USBLD && game->parts > 1) {
        guiMsgBox(_l(_STR_MMCE_ERR_UL_PARTS), 0, NULL);
        return;
    }

    if (mmcePort < 0) {
        guiMsgBox(_l(_STR_MMCE_ERR_NO_CARD), 0, NULL);
        return;
    }

    void *irx = &mmce_cdvdman_irx;
    int irx_size = size_mmce_cdvdman_irx;
    compatmask = sbPrepare(game, configSet, irx_size, irx, &index);
    settings = (struct cdvdman_settings_mmce *)((u8 *)irx + index);
    if (settings == NULL)
        return;

    // VMCs on the card are not served in game yet: say so instead of silently saving to the real card.
    for (vmc_id = 0; vmc_id < 2; vmc_id++) {
        configGetVMC(configSet, vmc_name, sizeof(vmc_name), vmc_id);
        if (vmc_name[0]) {
            guiWarning(_l(_STR_MMCE_NO_VMC_YET), 10);
            break;
        }
    }

    sbCreatePath(game, partname, mmcePrefix, "/", 0);

    fd = open(partname, O_RDONLY, 0666);
    if (fd < 0) {
        sbUnprepare(&settings->common);
        guiMsgBox(_l(_STR_ERR_FILE_INVALID), 0, NULL);
        return;
    }
    isoTotalBytes = lseek64(fd, 0, SEEK_END);
    if (gPS2Logo)
        EnablePS2Logo = CheckPS2Logo(fd, 0);
    close(fd);

    // Initialize layer 1 information.
    layer1_start = sbGetISO9660MaxLBA(partname);

    // Real media size, for CDVDMAN's out-of-bounds read emulation (see bdmsupport.c).
    settings->common.mediaLsnCount = sbGetMediaLsnCount(partname, isoTotalBytes);

    switch (game->format) {
        case GAME_FORMAT_USBLD:
            layer1_part = layer1_start / 0x80000;
            layer1_offset = layer1_start % 0x80000;
            sbCreatePath(game, partname, mmcePrefix, "/", layer1_part);
            break;
        default: // Raw ISO9660 disc image; one part.
            layer1_part = 0;
            layer1_offset = layer1_start;
    }

    if (sbProbeISO9660(partname, game, layer1_offset) != 0) {
        layer1_start = 0;
        LOG("DVD detected.\n");
    } else {
        layer1_start -= 16;
        LOG("DVD-DL layer 1 @ part %u sector 0x%lx.\n", layer1_part, layer1_offset);
    }
    settings->common.layer1_start = layer1_start;

    // No sector cache for compressed images on MMCE (as in the ps2-mmce fork).
    settings->common.zso_cache = 0;

    if ((result = sbLoadCheats(mmcePrefix, game->startup, configSet)) < 0) {
        switch (result) {
            case -ENOENT:
                guiWarning(_l(_STR_NO_CHEATS_FOUND), 10);
                break;
            default:
                guiWarning(_l(_STR_ERR_CHEATS_LOAD_FAILED), 10);
        }
    }

    if (gRememberLastPlayed) {
        configSetStr(configGetByType(CONFIG_LAST), "last_played", game->startup);
        saveConfig(CONFIG_LAST, 0);
    }

    if (configGetStrCopy(configSet, CONFIG_ITEM_ALTSTARTUP, filename, sizeof(filename)) == 0)
        strcpy(filename, game->startup);

    // The card keeps the image open across the IOP reset and mmcedrv reads it by the card's own
    // descriptor, which is neither the EE's nor the IOP's. The handle is not closed on purpose.
    sbCreatePath(game, partname, mmcePrefix, "/", 0);
    fd = open(partname, O_RDONLY, 0666);
    if (fd < 0) {
        sbUnprepare(&settings->common);
        guiMsgBox(_l(_STR_ERR_FILE_INVALID), 0, NULL);
        return;
    }
    iso_fd = fileXioIoctl2(ps2sdk_get_iop_fd(fd), MMCE_IOCTL2_GET_FD, NULL, 0, NULL, 0);
    if (iso_fd < 0) {
        close(fd);
        sbUnprepare(&settings->common);
        guiMsgBox(_l(_STR_ERR_FILE_INVALID), 0, NULL);
        return;
    }
    settings->port = mmcePort;
    settings->iso_fd = iso_fd;

    LOG("MMCESUPPORT %s: port %d, card fd %d\n", game->startup, mmcePort, iso_fd);

    deinit(NO_EXCEPTION, MMCE_MODE); // CAREFUL: deinit will call mmceCleanUp, so mmceGames/game will be freed

    sysLaunchLoaderElf(filename, "MMCE_MODE", irx_size, irx, 0, NULL, EnablePS2Logo, compatmask);
}

static config_set_t *mmceGetConfig(item_list_t *itemList, int id)
{
    return sbPopulateConfig(&mmceGames[id], mmcePrefix, "/", NULL);
}

static int mmceGetImage(item_list_t *itemList, char *folder, int isRelative, char *value, char *suffix, GSTEXTURE *resultTex, short psm)
{
    char path[256];
    if (isRelative)
        snprintf(path, sizeof(path), "%s%s/%s_%s", mmcePrefix, folder, value, suffix);
    else
        snprintf(path, sizeof(path), "%s%s_%s", folder, value, suffix);
    return texDiscoverLoad(resultTex, path, -1);
}

static int mmceGetTextId(item_list_t *itemList)
{
    return _STR_MMCE_GAMES;
}

static int mmceGetIconId(item_list_t *itemList)
{
    return MMCE_ICON;
}

// This may be called, even if mmceInit() was not.
static void mmceCleanUp(item_list_t *itemList, int exception)
{
    if (mmceGameList.enabled) {
        LOG("MMCESUPPORT CleanUp\n");

        free(mmceGames);
        mmceGames = NULL;
        mmceGameCount = 0;
    }
}

// This may be called, even if mmceInit() was not.
static void mmceShutdown(item_list_t *itemList)
{
    if (mmceGameList.enabled) {
        LOG("MMCESUPPORT Shutdown\n");

        free(mmceGames);
        mmceGames = NULL;
        mmceGameCount = 0;
    }
}

static int mmceCheckVMC(item_list_t *itemList, char *name, int createSize)
{
    return sysCheckVMC(mmcePrefix, "/", name, createSize, NULL);
}

static char *mmceGetPrefix(item_list_t *itemList)
{
    return mmcePrefix;
}

static item_list_t mmceGameList = {
    MMCE_MODE, 2, 0, 0, MENU_MIN_INACTIVE_FRAMES, -1, NULL, NULL, &mmceGetTextId, &mmceGetPrefix, &mmceInit, &mmceNeedsUpdate,
    &mmceUpdateGameList, &mmceGetGameCount, &mmceGetGame, &mmceGetGameName, &mmceGetGameNameLength, &mmceGetGameStartup, &mmceDeleteGame, &mmceRenameGame,
    &mmceLaunchGame, &mmceGetConfig, &mmceGetImage, &mmceCleanUp, &mmceShutdown, &mmceCheckVMC, &mmceGetIconId};
