/* Host harness for a theme change while a game list is shown. Every game item keeps one art cache id and UID per
 * cache of the theme (src/menusys.c submenuRebuildCache(), sized by gTheme->gameCacheCount); drawing a game image
 * (src/themes.c getGameImageTexture() -> src/texcache.c cacheGetTexture()) indexes them by the cache's userId and
 * writes through them. thmLoad() swaps the theme and frees the old one under guiLock(); the item arrays used to be
 * rebuilt only later, by moduleUpdateMenu() in a separate locked step. A GUI frame fits in between whenever the GUI
 * thread outranks the IO thread (priority 2 after an HD video mode, before 9ab1f447) or the IO thread blocks.
 *
 * The swap below mirrors the end of thmLoad(); it calls menuRebuildCaches() when the tree's thmLoad() does
 * (SWAP_REBUILDS, set by run_all.ps1). Usage: thmswap-t more | fewer
 *   more:  the new theme has 7 game art caches, the old one 5 (as the internal theme: COV, ICO, BG, SCR, SCR2)
 *   fewer: the new theme keeps 4 icons in its ICO cache, the old one 20 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LOG(...)

typedef unsigned char u8;
typedef struct
{
    int Width, Height, PSM, ClutPSM, TBW, Filter, ClutStorageMode, Delayed;
    void *Mem, *Clut;
    unsigned int Vram, VramClut;
} GSTEXTURE;
#define GS_PSM_CT24          1
#define GS_CLUT_STORAGE_CSM1 0

typedef struct item_list
{
    int delay;
    char *(*itemGetStartup)(struct item_list *list, int id);
    int (*itemGetImage)(struct item_list *list, char *folder, int isRelative, char *value, char *suffix, GSTEXTURE *resultTex, short psm);
} item_list_t;

typedef struct
{
    GSTEXTURE texture;
    void *qr;
    int lastUsed;
    int UID;
} cache_entry_t;

typedef struct
{
    int userId, count;
    char *prefix;
    int isPrefixRelative;
    char *suffix;
    int nextUID;
    cache_entry_t *content;
} image_cache_t;

typedef struct submenu_item
{
    int id;
    int *cache_id;
    int *cache_uid;
} submenu_item_t;
typedef struct submenu_list
{
    submenu_item_t item;
    struct submenu_list *prev, *next;
} submenu_list_t;
typedef struct menu_item
{
    struct submenu_list *submenu;
} menu_item_t;
typedef struct menu_list
{
    struct menu_item *item;
    struct menu_list *prev, *next;
} menu_list_t;

#define MAX_CACHES 8
typedef struct
{
    int gameCacheCount;
    image_cache_t *caches[MAX_CACHES];
} theme_t;

theme_t *gTheme;
int gEnableArt = 1;
int guiFrameId = 100, guiInactiveFrames = 100;
static menu_list_t *menu;
static int menuListSemaId = 1;
static void WaitSema(int id) {}
static void SignalSema(int id) {}

static void *requests[256];
static int nRequests;
#define IO_CACHE_LOAD_ART 3
static int ioPutRequest(int type, void *data)
{
    requests[nRequests++] = data; // the IO thread is busy elsewhere: the requests wait
    return 0;
}
static void ioRegisterHandler(int t, void *h) {}
static void guiLock(void) {}
static void guiUnlock(void) {}
static void rmUnloadTexture(GSTEXTURE *t) {}
static void texFree(GSTEXTURE *t) {}
static char *getStartup(item_list_t *list, int id) { return "SLUS_123.45"; }

// run_all.ps1 puts the functions of themes.c first
GSTEXTURE *cacheGetTexture(image_cache_t *cache, item_list_t *list, int *cacheId, int *UID, char *value);
void submenuRebuildCache(submenu_list_t *submenu);

#include "thmswap_extracted.c"

static theme_t *newTheme(int count, int icoCount)
{
    static const char *suffix[MAX_CACHES] = {"COV", "ICO", "BG", "SCR", "SCR2", "LGO", "LAB"};
    theme_t *theme = calloc(1, sizeof(theme_t));
    for (int i = 0; i < count; i++)
        theme->caches[i] = cacheInitCache(theme->gameCacheCount++, "ART", 1, suffix[i], i == 1 ? icoCount : (i == 0 ? 10 : 1));
    return theme;
}

static void drawFrame(item_list_t *list)
{
    for (submenu_list_t *it = menu->item->submenu; it; it = it->next)
        for (int c = 0; c < gTheme->gameCacheCount; c++)
            getGameImageTexture(gTheme->caches[c], list, &it->item);
}

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "more";
    item_list_t list = {0, &getStartup, NULL};
    submenu_list_t *games = NULL;
    theme_t *oldT, *newT;
    int i;

    // the internal theme and a list of 8 games, drawn once: every game gets cache ids
    gTheme = oldT = newTheme(5, 20);
    for (i = 7; i >= 0; i--) {
        submenu_list_t *it = calloc(1, sizeof(submenu_list_t));
        it->item.id = i;
        it->next = games;
        games = it;
    }
    submenuRebuildCache(games);
    menu = calloc(1, sizeof(menu_list_t));
    menu->item = calloc(1, sizeof(menu_item_t));
    menu->item->submenu = games;
    drawFrame(&list);

    // the IO thread: a device with the configured theme was found (end of thmLoad())
    newT = !strcmp(mode, "fewer") ? newTheme(5, 4) : newTheme(7, 20);
    guiLock();
    gTheme = newT;
#ifdef SWAP_REBUILDS
    menuRebuildCaches();
#endif
    for (i = 0; i < oldT->gameCacheCount; i++)
        cacheDestroyCache(oldT->caches[i]);
    free(oldT);
    guiUnlock();

    // a GUI frame before moduleUpdateMenu() rebuilds the items
    fflush(stdout);
    drawFrame(&list);
    printf("frame after the swap drawn, %d art requests\n", nRequests);

    submenuRebuildCache(games); // moduleUpdateMenu(i, 1, 0)
    return 0;
}
