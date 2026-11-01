/* Host harness for actions on the selected game that wait for a dialog or for the IO thread first: Delete
 * (src/menusys.c menuDeleteGame(), after the confirmation), Rename (menuRenameGame(), after the keyboard) and the
 * launch (src/opl.c itemExecSelect(), after menuLoadConfig() and the parental lock prompt). They read
 * selected_item->item->current once the wait is over. Meanwhile the IO thread may have reread the list (a device
 * pulled or plugged, a refresh): clearMenuGameList() destroys the items and sets current to NULL; the GUI thread
 * appends the new items only after the wait, but the IO thread applies the queued items itself before its next
 * reread (guiExecDeferredOps()), and the first of them is then the current item.
 *
 * Usage: stalesel-t delete-pulled | delete-reread | rename-pulled | launch-pulled | launch-loading | <action>-kept
 *   *-pulled:       the list is cleared during the wait (the device was pulled)
 *   delete-reread:  the list is reread with a new game in front, and its items applied, during the confirmation
 *   launch-loading: the list is cleared before the IO thread takes the settings request, and no settings are cached
 *   *-kept:         the list does not change
 * The menu is opened for Gamma, the third game. Prints what was deleted, renamed or launched; a NULL dereference ends
 * the run with a crash. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)

typedef struct
{
    const char *game;
} config_set_t;

typedef struct submenu_item
{
    int icon_id;
    char *text;
    int text_id;
    int id;
} submenu_item_t;
typedef struct submenu_list
{
    struct submenu_item item;
    struct submenu_list *prev, *next;
} submenu_list_t;
typedef struct menu_item
{
    void *userdata;
    struct submenu_list *submenu, *current;
} menu_item_t;
typedef struct menu_list
{
    struct menu_item *item;
} menu_list_t;

typedef struct item_list
{
    int mode, enabled, delay;
    int (*itemGetCount)(struct item_list *list);
    int (*itemGetNameLength)(struct item_list *list, int id);
    config_set_t *(*itemGetConfig)(struct item_list *list, int id);
    void (*itemRename)(struct item_list *list, int id, char *newName);
    void (*itemDelete)(struct item_list *list, int id);
    void (*itemLaunch)(struct item_list *list, int id, config_set_t *configSet);
} item_list_t;
typedef struct
{
    item_list_t *support;
} opl_io_module_t;

enum { SFX_CONFIRM,
       SFX_MESSAGE };
enum { GUI_SCREEN_MAIN };
enum { _STR_DELETE_WARNING = 1,
       _STR_LOADING_SETTINGS };
enum { BDM_MODE,
       BDM_MODE4 = 3 };
#define IO_CUSTOM_SIMPLEACTION  1
#define IO_MENU_UPDATE_DEFFERED 2

static int gEnableWrite = 1, guiInactiveFrames;
static menu_list_t *selected_item;
static config_set_t *itemConfig;
static int itemConfigId = -1, actionStatus, menuSemaId;
static opl_io_module_t list_support[4];

static const char *mode;
static const char *names[8];
static int count;
static char *result = "nothing";
static char resultText[64];

static char *_l(int id) { return "text"; }
static void sfxPlay(int id) {}
static void WaitSema(int id) {}
static void SignalSema(int id) {}
static void guiSwitchScreen(int to) {}
static void submenuDestroy(submenu_list_t **submenu) { *submenu = NULL; } // the dialog's own menu
static int menuCheckParentalLock(void) { return 0; }
static int menuCheckGameParentalLock(config_set_t *configSet) { return 0; }
static void itemInitSupport(item_list_t *support) {}
static void configFree(config_set_t *configSet) { free(configSet); }

// The support's game list and the menu's items, as updateMenuFromGameList() and guiHandleOp() build them
static menu_item_t games;
static void setList(int n, const char **list)
{
    for (count = 0; count < n; count++)
        names[count] = list[count];
}
static void appendItems(void)
{
    submenu_list_t *prev = NULL;
    for (int i = 0; i < count; i++) {
        submenu_list_t *it = calloc(1, sizeof(submenu_list_t));
        it->item.text = (char *)names[i];
        it->item.text_id = -1;
        it->item.id = i;
        it->prev = prev;
        if (prev)
            prev->next = it;
        else
            games.submenu = games.current = it;
        prev = it;
    }
}
static void clearItems(void)
{
    // clearMenuGameList()
    while (games.submenu) {
        submenu_list_t *next = games.submenu->next;
        free(games.submenu);
        games.submenu = next;
    }
    games.current = NULL;
}

// The IO thread's step during a wait
static int clearPending;
static void ioStep(void)
{
    static const char *reread[] = {"Aardvark", "Alpha", "Beta", "Gamma"};
    if (strstr(mode, "kept")) {
        // nothing changes
    } else if (!strcmp(mode, "delete-reread")) {
        clearItems();
        setList(4, reread);
        appendItems(); // guiExecDeferredOps() before the IO thread's next reread
    } else if (strstr(mode, "pulled") || clearPending) {
        clearItems();
        count = 0;
    }
    clearPending = 0;
}

static void (*queued[4])(void);
static int nQueued;
static void ioPutRequest(int type, void *data)
{
    if (type == IO_CUSTOM_SIMPLEACTION)
        queued[nQueued++] = (void (*)(void))data;
}
static void guiRenderTextScreen(const char *message)
{
    static int frames;
    if (clearPending)
        ioStep(); // a reread queued before the request
    while (nQueued > 0) {
        void (*handler)(void) = queued[0];
        memmove(queued, queued + 1, --nQueued * sizeof(queued[0]));
        handler();
    }
    if (++frames == 1000) {
        printf("launched: nothing, \"Loading settings\" still shown after %d frames\n", frames);
        exit(0);
    }
}

static int guiMsgBox(const char *text, int addAccept, void *ui)
{
    ioStep();
    return 1;
}
static int guiShowKeyboard(char *value, int maxLength)
{
    ioStep();
    strcpy(value, "Gamma 2");
    return 1;
}

static int gameCount(item_list_t *list) { return count; }
static int nameLength(item_list_t *list, int id) { return 32; }
static config_set_t *gameConfig(item_list_t *list, int id)
{
    config_set_t *configSet = malloc(sizeof(config_set_t));
    configSet->game = names[id];
    return configSet;
}
static void gameRename(item_list_t *list, int id, char *newName)
{
    snprintf(resultText, sizeof(resultText), "%s to %s", names[id], newName);
    result = resultText;
}
static void gameDelete(item_list_t *list, int id) { result = (char *)names[id]; }
static void gameLaunch(item_list_t *list, int id, config_set_t *configSet)
{
    snprintf(resultText, sizeof(resultText), "%s with the settings of %s", names[id], configSet->game);
    result = resultText;
}

// run_all.ps1 puts the functions of opl.c and gui.c first
config_set_t *menuLoadConfig();
int menuIsCurrentItem(menu_item_t *menu, int id, const char *name, int length);
void guiHandleDeferedIO(int *ptr, const char *message, int type, void *data);

#include "stalesel_extracted.c"

int main(int argc, char **argv)
{
    static const char *list[] = {"Alpha", "Beta", "Gamma", "Delta"};
    item_list_t support = {0, 1, 0, &gameCount, &nameLength, &gameConfig, &gameRename, &gameDelete, &gameLaunch};
    menu_list_t entry = {&games};
    submenu_list_t *menu = NULL;

    mode = argc > 1 ? argv[1] : "delete-pulled";
    games.userdata = &support;
    selected_item = &entry;
    setList(4, list);
    appendItems();
    games.current = games.current->next->next; // Gamma
    fflush(stdout);

    if (!strncmp(mode, "delete", 6)) {
        menuDeleteGame(&menu);
        printf("deleted: %s\n", result);
    } else if (!strncmp(mode, "rename", 6)) {
        menuRenameGame(&menu);
        printf("renamed: %s\n", result);
    } else {
        // the info page has cached the settings of the selected game, unless the cursor has just moved to it
        if (!strcmp(mode, "launch-pulled")) {
            itemConfig = gameConfig(&support, 2);
            itemConfigId = 2;
        }
        clearPending = 1;
        itemExecSelect(&games);
        printf("launched: %s\n", result);
    }
    return 0;
}
