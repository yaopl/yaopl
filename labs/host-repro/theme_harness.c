/* Host harness for theme elements of src/themes.c: the GameCountText element,
 * image textures and the items list. configGetInt() and configGetStr() find
 * nothing, as with a theme that sets no property; texDiscoverLoad() finds no
 * file and no internal texture has the name asked for. Modes: theme-t gamecount | image | internal | itemslist */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int u32;
typedef unsigned long long u64;
#define LOG(...)
#define DIM_UNDEF      -1
#define ALIGN_NONE     0
#define SCALING_NONE   0
#define SCALING_RATIO  1
#define SIZING_NONE    -1
#define SIZING_CLIP    0
#define SIZING_WRAP    1
#define DISPLAY_ALWAYS 0
#define DISPLAY_NEVER  2
#define THM_MAX_FONTS  16
enum {
    ELEM_TYPE_ATTRIBUTE_TEXT,
    ELEM_TYPE_STATIC_TEXT,
    ELEM_TYPE_ATTRIBUTE_IMAGE,
    ELEM_TYPE_GAME_IMAGE,
    ELEM_TYPE_STATIC_IMAGE,
    ELEM_TYPE_BACKGROUND,
    ELEM_TYPE_MENU_ICON,
    ELEM_TYPE_MENU_TEXT,
    ELEM_TYPE_ITEMS_LIST
};
enum {
    _STR_INFO_TITLE,
    _STR_INFO_GENRE,
    _STR_INFO_RELEASE,
    _STR_INFO_DEVELOPER,
    _STR_SIZE,
    _STR_INFO_DESCRIPTION,
    _STR_FILE_COUNT
};

typedef struct
{
    int uid;
} config_set_t;
typedef struct
{
    u32 Width, Height;
    void *Mem;
    void *Clut;
} GSTEXTURE;
typedef struct item_list
{
    int (*itemGetCount)(struct item_list *itemList);
} item_list_t;
struct menu_item
{
    void *userdata;
};
struct menu_list
{
    struct menu_item *item;
};
struct submenu_list;

typedef struct
{
    int upperLeft_x, upperLeft_y, upperRight_x, upperRight_y, lowerLeft_x, lowerLeft_y, lowerRight_x, lowerRight_y;
    char *name;
    GSTEXTURE source;
} image_texture_t;
typedef struct
{
    int count;
    char *suffix;
} image_cache_t;
typedef struct
{
    int currentUid;
    u32 currentConfigId;
    char *currentValue;
    image_cache_t *cache;
    int cacheLinked;
    image_texture_t *defaultTexture;
    image_texture_t *overlayTexture;
} mutable_image_t;
typedef struct
{
    char *value;
    int sizingMode;
    char *alias;
    int displayMode;
    u32 currentConfigId;
    char *currentValue;
    char *wrappedValue; /* the private copy a fix may add */
} mutable_text_t;
typedef struct
{
    int displayedItems;
    const char *decorator;
    mutable_image_t *decoratorImage;
} items_list_t;
typedef struct theme_element
{
    int type;
    int posX;
    int posY;
    short aligned;
    int width;
    int height;
    short scaled;
    u64 color;
    int font;
    void *extended;
    void (*drawElem)(struct menu_list *menu, struct submenu_list *item, config_set_t *config, struct theme_element *elem);
    void (*endElem)(struct theme_element *elem);
    struct theme_element *next;
} theme_element_t;
typedef struct
{
    theme_element_t *first;
    theme_element_t *last;
} theme_elems_t;
typedef struct theme
{
    int usedHeight;
    u64 textColor;
    theme_elems_t mainElems;
    theme_elems_t infoElems;
    theme_element_t *gamesItemsList;
    theme_elems_t appsMainElems;
    theme_elems_t appsInfoElems;
    theme_element_t *appsItemsList;
    int fonts[THM_MAX_FONTS];
} theme_t;

static int screenWidth = 640, screenHeight = 448;
static char *strings[] = {"Title", "Genre", "Release", "Developer", "Size", "Description", "%d games"};

char *_l(unsigned int id) { return strings[id]; }
int configGetInt(config_set_t *configSet, const char *key, int *value) { return 0; }
int configGetStr(config_set_t *configSet, const char *key, const char **value) { return 0; }
void fntFitString(int id, char *string, size_t width) {}
int fntRenderString(int id, int x, int y, short aligned, size_t width, size_t height, const char *string, u64 colour) { return 0; }
void rmUnloadTexture(GSTEXTURE *txt) {}
int texLookupInternalTexId(const char *name) { return -1; }
int texLoadInternal(GSTEXTURE *texture, int texId) { return -1; }
/* the file is not there: as in OPL, nothing is written to the texture */
int texDiscoverLoad(GSTEXTURE *texture, const char *path, int texId) { return -1; }
static void endMutableText(theme_element_t *elem) {}
static theme_element_t *initBasic(const char *themePath, config_set_t *themeConfig, theme_t *theme, const char *name, int type, int x, int y, short aligned, int w, int h, short scaled, u64 color, int font)
{
    theme_element_t *elem = calloc(1, sizeof(theme_element_t));
    elem->type = type;
    elem->width = w;
    elem->height = h;
    return elem;
}
static void initItemsList(const char *themePath, config_set_t *themeConfig, theme_t *theme, theme_element_t *elem, const char *name, const char *decorator)
{
    items_list_t *itemsList = calloc(1, sizeof(items_list_t));
    itemsList->displayedItems = 10;
    elem->extended = itemsList;
}
static void validateBackgroundElems(const char *themePath, config_set_t *themeConfig, theme_t *theme, theme_elems_t *mainElems, theme_elems_t *infoElems) {}

#include "theme_extracted.c"

static int countGames(item_list_t *itemList)
{
    return 1234;
}

int main(int argc, char **argv)
{
    theme_t theme;

    memset(&theme, 0, sizeof(theme));
    if (argc == 2 && strcmp(argv[1], "gamecount") == 0) {
        item_list_t list = {countGames};
        struct menu_item menuItem = {&list};
        struct menu_list menu = {&menuItem};
        config_set_t config = {1};
        theme_element_t elem;

        memset(&elem, 0, sizeof(elem));
        elem.width = DIM_UNDEF;
        elem.height = DIM_UNDEF;
        initGameCountText(NULL, NULL, &theme, &elem, "gc");
        elem.drawElem(&menu, NULL, &config, &elem);
        printf("GameCountText value: \"%s\"\n", ((mutable_text_t *)elem.extended)->value);
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "image") == 0) {
        image_texture_t *texture = initImageTexture("mass:/THM/thm_x/", NULL, "bg", "missing", 0);
        printf("image file missing: %s\n", texture ? "texture kept, its memory pointer never set" : "no texture, element disabled");
        fflush(stdout);
        if (texture)
            freeImageTexture(texture);
        printf("freed\n");
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "internal") == 0) {
        // Leave a used block of the texture's size for malloc() to hand out again, as the menu's heap does
        void *used = malloc(sizeof(image_texture_t));
        memset(used, 0xAB, sizeof(image_texture_t));
        free(used);

        image_texture_t *texture = initImageInternalTexture(NULL, "nosuch");
        printf("internal texture unknown: %s\n", texture ? "texture kept" : "no texture, element disabled");
        fflush(stdout);
        if (texture)
            freeImageTexture(texture);
        printf("freed\n");
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "itemslist") == 0) {
        theme_element_t background, appsBackground;

        memset(&background, 0, sizeof(background));
        memset(&appsBackground, 0, sizeof(appsBackground));
        background.type = appsBackground.type = ELEM_TYPE_BACKGROUND;
        theme.mainElems.first = theme.mainElems.last = &background;
        theme.appsMainElems.first = theme.appsMainElems.last = &appsBackground;
        validateGUIElems("mass:/THM/thm_x/", NULL, &theme);
        printf("theme without ItemsList: gamesItemsList %s, appsItemsList %s\n",
               theme.gamesItemsList ? "set" : "NULL (menusys dereferences it)", theme.appsItemsList ? "set" : "NULL");
        return 0;
    }
    return 2;
}
