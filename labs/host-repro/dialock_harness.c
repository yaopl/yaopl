/* Host harness for the frames of src/dia.c: diaExecuteDialog() (every settings dialog), diaShowKeyb() (the
 * on-screen keyboard) and diaShowColSel() (the colour picker). While a dialog is open, the IO thread (priority 30)
 * may load a theme or a language found on a device: it frees the old one under guiLock() (thmLoad(),
 * lngLoadFromFile()). It preempts the GUI thread (priority 31) the moment it becomes ready, i.e. anywhere in a
 * frame; it then waits only if the GUI thread holds guiLock(). Here every draw call is such a moment: the IO
 * step runs there if the lock is free, else at the guiUnlock() that frees it. The step swaps the language strings
 * and the theme and frees the old ones; the draw call then reads the string or the texture it was given.
 *
 * Usage: dialock-t dialog|keyboard|colour
 * Prints the frames drawn and how many of them did not hold the lock; ASan reports a draw from freed memory. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)

typedef unsigned long long u64;
typedef struct
{
    int Width, Height;
} GSTEXTURE;
typedef struct
{
    int fonts[1];
    u64 textColor, uiTextColor, selTextColor;
    GSTEXTURE textures[8];
} theme_t;
struct UIItem
{
    int type, id;
};
enum { UI_TERMINATOR,
       UI_BUTTON,
       UI_OK,
       UI_INT };
#define UIID_BTN_OK                1
#define UIID_BTN_CANCEL            2
#define GS_SETREG_RGBA(r, g, b, a) ((u64)(r) | ((u64)(g) << 8) | ((u64)(b) << 16) | ((u64)(a) << 24))
enum { KEY_LEFT = 1,
       KEY_RIGHT,
       KEY_UP,
       KEY_DOWN,
       KEY_CROSS,
       KEY_CIRCLE,
       KEY_SQUARE,
       KEY_TRIANGLE,
       KEY_START,
       KEY_SELECT };
enum { SFX_CURSOR,
       SFX_CONFIRM,
       SFX_CANCEL };
enum { _STR_BACKSPACE,
       _STR_SPACE,
       _STR_ENTER,
       _STR_MODE,
       _STR_CANCEL,
       _STR_COLOR_SELECTION,
       _STR_OK,
       LANG_STR_COUNT };
enum { SQUARE_ICON,
       TRIANGLE_ICON,
       START_ICON,
       SELECT_ICON,
       CIRCLE_ICON,
       CROSS_ICON };
#define ALIGN_NONE    0
#define ALIGN_VCENTER 2
#define SCALING_RATIO 1

static u64 gDefaultCol, gColDarker, gColWhite, gColFocus;
static int gSelectButton = KEY_CROSS;
static int screenWidth, screenHeight;
static int colPadSettings[16];

// The theme and the language the IO thread swaps
static theme_t *gTheme;
static char **lang_strs;
static void loadLanguage(void)
{
    char **strs = malloc(LANG_STR_COUNT * sizeof(char *));
    for (int i = 0; i < LANG_STR_COUNT; i++)
        strs[i] = _strdup("Text");
    lang_strs = strs;
}
static void loadTheme(void)
{
    theme_t *theme = calloc(1, sizeof(theme_t));
    for (int i = 0; i < 8; i++)
        theme->textures[i].Width = theme->textures[i].Height = 32;
    gTheme = theme;
}
static char *_l(unsigned int id) { return lang_strs[id]; }
static GSTEXTURE *thmGetTexture(unsigned int id) { return &gTheme->textures[id]; }

// The GUI lock and the IO thread
static int lockHeld, ioPending, ioDone, frames, unlockedFrames;
static void ioStep(void)
{
    char **oldL = lang_strs;
    theme_t *oldT = gTheme;
    loadLanguage();
    loadTheme();
    for (int i = 0; i < LANG_STR_COUNT; i++)
        free(oldL[i]);
    free(oldL);
    free(oldT);
    ioPending = 0;
    ioDone = 1;
}
static void preempt(void)
{
    if (ioPending && !lockHeld)
        ioStep();
}
static void guiLock(void) { lockHeld = 1; }
static void guiUnlock(void)
{
    lockHeld = 0;
    preempt(); // the IO thread waits for the lock and runs as soon as it is free
}

// Drawing: the IO thread can run in any draw call that was given a string or a texture, before it reads it
static void rmStartFrame(void)
{
    frames++;
    if (!lockHeld)
        unlockedFrames++;
    if (frames == 2)
        ioPending = 1; // the device with the theme and the language is found during the second frame
}
static void rmEndFrame(void) { preempt(); }
static void rmGetScreenExtents(int *w, int *h) { *w = 640, *h = 480; }
static void rmDrawRect(int x, int y, int w, int h, u64 color) {}
static void rmDrawLine(int x1, int y1, int x2, int y2, u64 color) {}
static volatile int sink;
static void rmDrawPixmap(GSTEXTURE *txt, int x, int y, short aligned, int w, int h, short scaled, u64 color)
{
    preempt();
    sink += txt->Width;
}
static int fntRenderString(int id, int x, int y, short aligned, size_t width, size_t height, const char *string, u64 colour)
{
    preempt();
    sink += (int)strlen(string);
    return x + 8;
}
static void guiDrawBGPlasma(void) {}
static int guiDrawIconAndText(int iconId, int textId, int font, int x, int y, u64 color)
{
    rmDrawPixmap(thmGetTexture(iconId), x, y, ALIGN_NONE, 0, 0, SCALING_RATIO, color);
    return fntRenderString(font, x, y, ALIGN_NONE, 0, 0, _l(textId), color);
}
static void diaRenderUI(struct UIItem *ui, short inMenu, struct UIItem *cur, int haveFocus)
{
    guiDrawBGPlasma();
    fntRenderString(gTheme->fonts[0], 20, 20, ALIGN_NONE, 0, 0, _l(_STR_OK), gTheme->textColor);
    guiDrawIconAndText(CIRCLE_ICON, _STR_CANCEL, gTheme->fonts[0], 70, 417, gTheme->textColor);
}

// Input: the user cancels in the fourth frame
static int padFrames;
static void readPads(void) { padFrames++; }
static int getKey(int key) { return padFrames >= 4 && key == KEY_CIRCLE; }
static int getKeyOn(int key) { return getKey(key); }
static void sfxPlay(int id) {}
static void setButtonDelay(int key, int delay) {}
static void padStoreSettings(int *buffer) {}
static void padRestoreSettings(int *buffer) {}

// Dialog navigation
static struct UIItem items[3] = {{UI_INT, 10}, {UI_OK, 11}, {UI_TERMINATOR, 0}}; // the first is a label
static struct UIItem *diaFindByID(struct UIItem *ui, int id) { return NULL; }
static struct UIItem *diaGetFirstControl(struct UIItem *ui) { return &ui[1]; }
static struct UIItem *diaGetLastControl(struct UIItem *ui) { return &ui[1]; }
static struct UIItem *diaGetNextControl(struct UIItem *cur, struct UIItem *dflt) { return cur; }
static struct UIItem *diaGetPrevControl(struct UIItem *cur, struct UIItem *ui) { return cur; }
static struct UIItem *diaGetNextLine(struct UIItem *cur, struct UIItem *ui) { return cur; }
static struct UIItem *diaGetPrevLine(struct UIItem *cur, struct UIItem *ui) { return cur; }
static int diaHandleInput(struct UIItem *item, int *modified) { return 0; }
static void diaStoreScrollSpeed(void) {}
static void diaRestoreScrollSpeed(void) {}

#include "dialock_extracted.c"

int main(int argc, char **argv)
{
    const char *what = argc > 1 ? argv[1] : "dialog";
    loadLanguage();
    loadTheme();
    if (!strcmp(what, "keyboard")) {
        char text[32] = "name";
        diaShowKeyb(text, sizeof(text), 0, NULL);
    } else if (!strcmp(what, "colour")) {
        unsigned char r = 1, g = 2, b = 3;
        diaShowColSel(&r, &g, &b);
    } else
        diaExecuteDialog(items, -1, 1, NULL);
    printf("%d frames, %d without the GUI lock, theme and language %s\n", frames, unlockedFrames,
           ioDone ? "swapped" : "not swapped");
    return 0;
}
