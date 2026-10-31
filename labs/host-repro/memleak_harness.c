/* Host harness for allocations the menu keeps or uses after freeing them. Built with one of:
 *   /DAPPS  src/appsupport.c
 *     memleak-apps refresh <refreshes> <apps>
 *         appNeedsUpdate() + appUpdateItemList() as menuDeferredUpdate() runs them after another page
 *         updated; prints the allocations left (the list and the legacy apps config: 3)
 *     memleak-apps launch
 *         appLaunchItem() of an app with an argument; deinit() frees the item config as menuEnd() does,
 *         then the ELF loader gets the argument
 *   /DSB    src/supportbase.c
 *     memleak-sb <iso games>
 *         sbReadList() without ul.cfg when the list allocation fails; prints the scanned nodes left */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned int u32;
#define LOG(...)
#define O_RDONLY 0

static long liveAllocs;
static size_t failSize; // an allocation of exactly this size fails (0: none)
// The size is kept in front of the block, so that free() can fill it with 'X': memory read after it is
// freed then shows the same in the plain build on every machine (the ASan build reports the read)
static void *harness_malloc(size_t n)
{
    size_t *p;
    if (failSize && n == failSize)
        return NULL;
    liveAllocs++;
    p = malloc(n + 16);
    *p = n;
    return (char *)p + 16;
}
static void harness_free(void *p)
{
    if (!p)
        return;
    liveAllocs--;
    p = (char *)p - 16;
    memset((char *)p + 16, 'X', *(size_t *)p);
    free(p);
}

#ifdef APPS
#define APP_TITLE_MAX          128
#define APP_PATH_MAX           128
#define APP_BOOT_MAX           64
#define APP_ARGV1_MAX          128
#define CONFIG_KEY_NAME_LEN    32
#define CONFIG_KEY_VALUE_LEN   256
#define CONFIG_ITEM_STARTUP    "$Startup"
#define CONFIG_ITEM_ALTSTARTUP "$AltStartup"
#define UNMOUNT_EXCEPTION      0x01
#define _STR_ERR_FILE_INVALID  0
#define BDM_MODE4              4
#define HDD_MODE               6
#define APP_MODE               7
typedef struct
{
    char title[APP_TITLE_MAX + 1];
    char path[APP_PATH_MAX + 1];
    char boot[APP_BOOT_MAX + 1];
    char argv1[APP_ARGV1_MAX + 1];
    u8 legacy;
} app_info_t;
// as include/config.h
struct config_value_t
{
    char key[CONFIG_KEY_NAME_LEN];
    char val[CONFIG_KEY_VALUE_LEN];
    struct config_value_t *next;
};
typedef struct
{
    struct config_value_t *head;
    char *filename;
} config_set_t;
typedef struct
{
    int dummy;
} item_list_t;

static config_set_t *configAlloc(const char *fileName)
{
    config_set_t *c = harness_malloc(sizeof(config_set_t));
    c->head = NULL;
    c->filename = harness_malloc(strlen(fileName) + 1);
    strcpy(c->filename, fileName);
    return c;
}
static void configSetStr(config_set_t *c, const char *key, const char *val)
{
    struct config_value_t *v = harness_malloc(sizeof(*v));
    snprintf(v->key, sizeof(v->key), "%s", key);
    snprintf(v->val, sizeof(v->val), "%s", val);
    v->next = c->head;
    c->head = v;
}
static void configClear(config_set_t *c)
{
    while (c->head) {
        struct config_value_t *v = c->head;
        c->head = v->next;
        harness_free(v);
    }
}
static void configFree(config_set_t *c)
{
    configClear(c);
    harness_free(c->filename);
    harness_free(c);
}
static int configRead(config_set_t *c) { return 0; } // no legacy conf_apps.cfg entries
static int configGetStr(config_set_t *c, const char *key, const char **value)
{
    struct config_value_t *v;
    for (v = c->head; v; v = v->next) {
        if (!strcmp(v->key, key)) {
            *value = v->val;
            return 1;
        }
    }
    return 0;
}
static int configGetStrCopy(config_set_t *c, const char *key, char *value, int length)
{
    const char *valref = NULL;
    if (configGetStr(c, key, &valref)) {
        strncpy(value, valref, length);
        value[length - 1] = '\0';
        return 1;
    }
    value[0] = '\0';
    return 0;
}
static config_set_t *oplGetLegacyAppsConfig(void) { return configAlloc("mass0:conf_apps.cfg"); }
static int shouldAppsUpdate;
static int oplShouldAppsUpdate(void)
{
    int r = shouldAppsUpdate;
    shouldAppsUpdate = 0;
    return r;
}
static const char *appGetELFName(const char *path) { return path; }

static int appForceUpdate = 1;
static int appItemCount = 0;
static config_set_t *configApps;
static app_info_t *appsList;
struct app_info_linked
{
    struct app_info_linked *next;
    app_info_t app;
};
static int nScanApps;
typedef int (*app_scan_callback_t)(const char *path, config_set_t *appConfig, void *arg);
// oplScanApps() with appScanCallback(): one node per APPS/<folder>/title.cfg found on the devices
static int oplScanApps(app_scan_callback_t cb, void *arg)
{
    struct app_info_linked **list = arg;
    int i;
    for (i = 0; i < nScanApps; i++) {
        struct app_info_linked *app = harness_malloc(sizeof(*app));
        memset(app, 0, sizeof(*app));
        snprintf(app->app.title, sizeof(app->app.title), "App %d", i);
        app->next = *list;
        *list = app;
    }
    return nScanApps;
}
static int appScanCallback(const char *path, config_set_t *appConfig, void *arg) { return 0; }
static void appFreeList(void);

// the launch
static char gOPLPart[128];
static config_set_t *itemConfig; // the menu's config of the selected item
static int open(const char *path, int flags, ...) { return 3; }
static int close(int fd) { return 0; }
static int oplPath2Mode(const char *path) { return 0; }
static char *_l(int id) { return "file invalid"; }
static int guiMsgBox(const char *text, int addAccept, void *menu) { return 0; }
static void deinit(int exception, int modeSelected)
{
    // menuEnd()
    configFree(itemConfig);
    itemConfig = NULL;
}
static void LoadELFFromFileWithPartition(const char *filename, const char *partition, int argc, char *argv[])
{
    printf("%s, argc %d, argument \"%.8s\"\n", filename, argc, argc ? argv[0] : "");
}

#define malloc harness_malloc
#define free   harness_free
#include "memleak_extracted.c"
#undef malloc
#undef free

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0); // no stdout buffer allocated into a freed block
    if (argc > 3 && !strcmp(argv[1], "refresh")) {
        int refreshes = atoi(argv[2]), i;
        nScanApps = atoi(argv[3]);
        configApps = oplGetLegacyAppsConfig(); // appInit()
        for (i = 1; i <= refreshes; i++) {
            item_list_t list;
            shouldAppsUpdate = 1; // another page updated (menuDeferredUpdate)
            if (appNeedsUpdate(&list))
                appUpdateItemList(&list);
        }
        printf("%d apps listed, %ld allocations live (the list and its config: 3)\n", appItemCount, liveAllocs);
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "launch")) {
        item_list_t list;
        itemConfig = configAlloc("mass0:APPS/x/title.cfg");
        configSetStr(itemConfig, CONFIG_ITEM_STARTUP, "mass0:APPS/x/BOOT.ELF");
        configSetStr(itemConfig, CONFIG_ITEM_ALTSTARTUP, "-arg");
        appLaunchItem(&list, 0, itemConfig);
        return 0;
    }
    return 2;
}
#endif

#ifdef SB
#define UL_GAME_NAME_MAX       32
#define ISO_GAME_NAME_MAX      160
#define ISO_GAME_EXTENSION_MAX 4
#define GAME_STARTUP_MAX       12
#define GAME_FORMAT_USBLD      0
#define SCECdPS2CD             0x12
#define SCECdPS2DVD            0x14
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
    char name[UL_GAME_NAME_MAX];
    char magic[3];
    char startup[GAME_STARTUP_MAX];
    u8 parts;
    u8 media;
    u8 unknown[4];
    u8 Byte08;
    u8 unknown2[10];
} USBExtreme_game_entry_t;
static int openFile(const char *p, int f) { return -1; } // no ul.cfg
static int getFileSize(int fd) { return 0; }
static int read(int fd, void *b, int n) { return 0; }
static int close(int fd) { return 0; }
struct game_list_t;
static int scanForISO(char *path, char type, struct game_list_t **glist);

#define malloc harness_malloc
#define free   harness_free
#include "memleak_extracted.c"
#undef malloc
#undef free

static int nIso;
// scanForISO(): one node per ISO in DVD/
static int scanForISO(char *path, char type, struct game_list_t **glist)
{
    int i;
    if (type != SCECdPS2DVD)
        return 0;
    for (i = 0; i < nIso; i++) {
        struct game_list_t *g = harness_malloc(sizeof(*g));
        memset(g, 0, sizeof(*g));
        g->next = *glist;
        *glist = g;
    }
    return nIso;
}

int main(int argc, char **argv)
{
    base_game_info_t *list = NULL;
    int fsize, count;
    nIso = atoi(argv[1]);
    failSize = sizeof(base_game_info_t) * nIso;
    count = sbReadList(&list, "mass0:", &fsize, &count);
    printf("%d games listed, %ld scanned nodes left\n", count, liveAllocs);
    return 0;
}
#endif
