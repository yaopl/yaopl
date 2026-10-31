/* Host harness for src/menusys.c _menuLoadConfig() with src/bdmsupport.c bdmGetConfig() and
 * src/supportbase.c sbPopulateConfig(): the settings of the highlighted game, loaded in the IO thread
 * after a list update that was queued before them. The menu was built from 50 games and game 40 is
 * highlighted; the update rereads the list first. Argument: "same" (still 50 games), "shorter" (10 games)
 * or "removed" (the device is gone: no list). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define LOG(...)
typedef unsigned char u8;
typedef unsigned int u32;

#define ISO_GAME_NAME_MAX      160
#define ISO_GAME_EXTENSION_MAX 4
#define GAME_STARTUP_MAX       12
#define GAME_FORMAT_USBLD      0
#define GAME_FORMAT_OLD_ISO    1
#define GAME_FORMAT_ISO        2
#define SCECdPS2CD             0x12
#define MODE_FLAG_READ_ONLY    0x10
#define CONFIG_GAME            2
#define CONFIG_ITEM_NAME       "#Name"
#define CONFIG_ITEM_SIZE       "#Size"
#define CONFIG_ITEM_FORMAT     "#Format"
#define CONFIG_ITEM_MEDIA      "#Media"
#define CONFIG_ITEM_STARTUP    "#Startup"

typedef struct
{
    char name[ISO_GAME_NAME_MAX + 1];
    char startup[GAME_STARTUP_MAX + 1];
    char extension[ISO_GAME_EXTENSION_MAX + 1];
    u8 parts;
    u8 media;
    u8 format;
    u32 sizeMB;
} base_game_info_t;

typedef struct
{
    char *filename;
} config_set_t;

typedef struct
{
    char bdmPrefix[40];
    base_game_info_t *bdmGames;
    int bdmGameCount;
} bdm_device_data_t;

typedef struct item_list
{
    int flags;
    void *priv;
    int (*itemGetCount)(struct item_list *list);
    config_set_t *(*itemGetConfig)(struct item_list *list, int id);
} item_list_t;

typedef struct
{
    void *userdata;
} menu_item_t;
typedef struct
{
    menu_item_t *item;
} menu_list_t;

static config_set_t cfg = {"mass0:CFG/x.cfg"};
static config_set_t *configAlloc(int t, config_set_t *c, char *f) { return &cfg; }
static int configRead(config_set_t *c) { return 1; }
static void configMove(config_set_t *c, const char *f) {}
static config_set_t *configGetByType(int t) { return &cfg; }
static int configSetStr(config_set_t *c, const char *k, const char *v) { return (int)strlen(v); }
static int configSetInt(config_set_t *c, const char *k, int v) { return 1; }
static unsigned int USBA_crc32(const char *s) { return 0; } // upstream builds the ul.* path itself
static void sbCreatePath(const base_game_info_t *g, char *path, const char *p, const char *s, int part) { snprintf(path, 256, "%s%s", p, g->name); }
static void WaitSema(int id) {}
static void SignalSema(int id) {}

static menu_list_t *selected_item;
static int itemConfigId, actionStatus, menuSemaId;
static config_set_t *itemConfig;

static int bdmGetGameCount(item_list_t *list) { return ((bdm_device_data_t *)list->priv)->bdmGameCount; }

#include "staleid_extracted.c"

int main(int argc, char **argv)
{
    static bdm_device_data_t dev = {"mass0:"};
    item_list_t list = {0, &dev, &bdmGetGameCount, &bdmGetConfig};
    menu_item_t item = {&list};
    menu_list_t sel = {&item};
    const char *c = argc > 1 ? argv[1] : "same";
    int i, count = 0;

    if (!strcmp(c, "same"))
        count = 50;
    else if (!strcmp(c, "shorter"))
        count = 10;
    selected_item = &sel;
    itemConfigId = 40;

    dev.bdmGameCount = count;
    dev.bdmGames = count ? calloc(count, sizeof(base_game_info_t)) : NULL;
    for (i = 0; i < count; i++) {
        snprintf(dev.bdmGames[i].name, sizeof(dev.bdmGames[i].name), "Game %d", i);
        dev.bdmGames[i].parts = 1;
    }

    _menuLoadConfig();
    printf("%d games: settings of game 40 %s\n", count, itemConfig ? "loaded" : "not loaded");
    return 0;
}
