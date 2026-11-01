/* Host harness for the menu of an app (Triangle on an app in the Apps list: Rename, Delete), src/menusys.c
 * menuRenderAppMenu() and menuHandleInputAppMenu(). The IO thread rereads the apps list after any other list changed
 * (a USB stick plugged, SMB coming up, the periodic refresh): clearMenuGameList() (src/opl.c) destroys the items and
 * sets the list's current item to NULL under guiLock(), between two frames, and queues the new items, which the GUI
 * thread appends in guiHandleDeferredOps() after it has drawn the next frame; the first of them becomes the current
 * item. The loop below mirrors guiMainLoop(): draw, deferred ops, input (not while a screen transition runs).
 *
 * Usage: appmenu-t
 * The menu is opened for the third app; the list is reread; the user then presses Cross on Delete. Prints the screen
 * shown at the end and the app that was deleted, if any; a NULL dereference ends the run with a crash. Skipping the
 * frame with no current item is not enough: the menu then stays open on the first app of the reread list. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)

typedef unsigned long long u64;
typedef struct
{
    int fonts[1];
    int usedHeight;
    u64 textColor, selTextColor;
} theme_t;

typedef struct submenu_item
{
    int icon_id;
    char *text;
    int text_id;
    int id;
    int *cache_id;
    int *cache_uid;
} submenu_item_t;
typedef struct submenu_list
{
    struct submenu_item item;
    struct submenu_list *prev, *next;
} submenu_list_t;
typedef struct menu_item
{
    void *userdata;
    struct submenu_list *submenu, *current, *pagestart;
} menu_item_t;
typedef struct menu_list
{
    struct menu_item *item;
} menu_list_t;

typedef struct item_list
{
    int mode;
    int (*itemGetNameLength)(struct item_list *list, int id);
    void (*itemRename)(struct item_list *list, int id, char *newName);
    void (*itemDelete)(struct item_list *list, int id);
} item_list_t;

enum { ALIGN_CENTER = 1 };
enum { SFX_CURSOR,
       SFX_CONFIRM,
       SFX_MESSAGE };
enum { KEY_UP = 1,
       KEY_DOWN,
       KEY_CROSS,
       KEY_CIRCLE,
       KEY_START };
enum { GUI_SCREEN_MAIN,
       GUI_SCREEN_APP_MENU };
enum { _STR_RENAME = 1,
       _STR_DELETE,
       _STR_DELETE_WARNING };
#define IO_MENU_UPDATE_DEFFERED 2

static theme_t theme = {{0}, 480, 1, 2};
static theme_t *gTheme = &theme;
static int gEnableWrite = 1, gSelectButton = KEY_CROSS;
static submenu_list_t *appMenu, *appMenuCurrent;
static menu_list_t *selected_item;

static int screen = GUI_SCREEN_APP_MENU, target = -1, pressed, deletedId = -1;

static char *_l(int id) { return id == _STR_RENAME ? "Rename" : "Delete"; }
static void guiDrawBGPlasma(void) {}
static void guiDrawSubMenuHints(void) {}
static void fntRenderString(int font, int x, int y, int align, int w, int h, const char *string, u64 colour) { (void)strlen(string); }
static void sfxPlay(int id) {}
static void readPads(void) {}
static int getKey(int key) { return pressed == key; }
static int getKeyOn(int key) { return pressed == key; }
static int menuCheckParentalLock(void) { return 0; }
static int guiMsgBox(const char *text, int addAccept, void *ui) { return 1; }
static int guiShowKeyboard(char *value, int maxLength) { return 0; }
static void ioPutRequest(int type, void *data) {}
static void submenuRebuildCache(submenu_list_t *submenu) {}
static void guiSwitchScreen(int to)
{
    if (target < 0)
        target = to;
}
static int appGetNameLength(item_list_t *list, int id) { return 32; }
static void appDelete(item_list_t *list, int id) { deletedId = id; }

// run_all.ps1 puts the functions in source order
void submenuDestroy(submenu_list_t **submenu);
submenu_list_t *submenuAppendItem(submenu_list_t **submenu, int icon_id, char *text, int id, int text_id);
char *submenuItemGetText(submenu_item_t *it);

#include "appmenu_extracted.c"

static char *names[] = {"FreeMcBoot", "OPL", "wLaunchELF", "uLaunchELF"};

static void appendApps(menu_item_t *apps)
{
    // GUI_OP_APPEND_MENU for each app (src/gui.c guiHandleOp()): the first one becomes the current item
    for (int i = 0; i < 4; i++) {
        submenu_list_t *it = submenuAppendItem(&apps->submenu, -1, names[i], i, -1);
        if (i == 0)
            apps->current = apps->pagestart = it;
    }
}

static menu_item_t *appsItem;

static void frame(int key, int reread)
{
    if (screen == GUI_SCREEN_APP_MENU)
        menuRenderAppMenu();
    if (reread)
        appendApps(appsItem); // guiHandleDeferredOps()
    pressed = key;
    if (target >= 0) { // the transition: no input
        screen = target;
        target = -1;
    } else if (screen == GUI_SCREEN_APP_MENU)
        menuHandleInputAppMenu();
}

int main(void)
{
    item_list_t support = {0, &appGetNameLength, NULL, &appDelete};
    menu_item_t apps = {&support, NULL, NULL, NULL};
    menu_list_t entry = {&apps};

    appendApps(&apps);
    apps.current = apps.current->next->next; // Triangle on wLaunchELF
    selected_item = &entry;
    appsItem = &apps;
    menuInitAppMenu();

    frame(KEY_DOWN, 0); // to Delete

    // the IO thread rereads the apps list between two frames: clearMenuGameList(), then the append ops
    submenuDestroy(&apps.submenu);
    apps.current = apps.pagestart = NULL;
    fflush(stdout);
    frame(0, 1);

    frame(KEY_CROSS, 0); // Delete, confirmed
    frame(0, 0);

    printf("%s shown, %s%s\n", screen == GUI_SCREEN_MAIN ? "the apps list" : "the app menu", deletedId < 0 ? "nothing deleted" : "deleted: ",
           deletedId < 0 ? "" : names[deletedId]);
    return 0;
}
