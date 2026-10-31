/* Host harness for the theme and language lists of src/themes.c and src/lang.c: the
 * same theme or language on several devices, and a device that goes away. The device
 * code is mirrored here: at start the memory card's OPL folder is listed and the
 * settings applied (as lngInit(), thmInit() and _loadConfig()); a USB drive or an SMB
 * share adds its THM and LNG folders (bdmNeedsUpdate(), ethInitSMB()); a USB drive
 * that goes away and an SMB reconnect call what the tree has for that, as does a device
 * whose prefix changes (the tree's sbDropThemesLanguages() where it has one). In "dialog",
 * Display Settings keeps the lists it was given while a USB drive comes and goes, and then
 * applies the theme and language that were picked before.
 * Mode: thmlist-t <scenario>. Each step prints the theme list, the language list and
 * what is loaded, as name@device; "!" marks an entry or a loaded file whose device is
 * gone. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)
#define strcasecmp         _stricmp
#define DT_DIR             4
#define DT_REG             8
#define THM_MAX_FILES      64
#define MAX_LANGUAGE_FILES 15
#define MODE_COUNT         4
#define CONFIG_OPL         1
#define LANG_STR_COUNT     1

typedef struct
{
    char *filePath;
    char *name;
} device_file_t;
typedef device_file_t theme_file_t;
typedef device_file_t language_t;

typedef struct
{
    int dummy;
} config_set_t;

typedef struct
{
    int dummy;
} theme_t;

/* --- what the harness simulates --- */
static const char *home = "mc0:";   // the device that holds the settings
static const char *cfgTheme = NULL; // the theme and language in the settings
static const char *cfgLang = NULL;
static const char *loadedThm = NULL; // what thmLoad() and lngLoadFromFile() read last
static const char *loadedLng = NULL;

typedef struct
{
    const char *dir;
    const char *entries[4];
} sim_dir_t;

static sim_dir_t simDirs[8];
static int nSimDirs = 0;
static const char *gone[4]; // devices that went away
static int nGone = 0;

static int isGone(const char *path)
{
    int i;
    for (i = 0; i < nGone; i++)
        if (!strncmp(path, gone[i], strlen(gone[i])))
            return 1;
    return 0;
}

/* --- stubs for what the extracted code calls --- */
static theme_t *gTheme;
static config_set_t configOPLSet;
char *internalEnglish[LANG_STR_COUNT] = {"English"};

config_set_t *configGetByType(int type) { return &configOPLSet; }
char *configGetDir(void) { return (char *)home; }
int configGetStr(config_set_t *set, const char *key, const char **value)
{
    const char *v = NULL;
    if (!strcmp(key, "theme"))
        v = cfgTheme;
    if (!strcmp(key, "language_text"))
        v = cfgLang;
    if (v == NULL)
        return 0;
    *value = v;
    return 1;
}
void moduleUpdateMenu(int mode, int themeChanged, int langChanged) {}
static void thmLoad(const char *themePath) { loadedThm = themePath; }
static void thmSetColors(theme_t *theme) {}
static void bgmMute(void) {}
static void bgmUnMute(void) {}
static int fntLoadDefault(const char *path) { return 0; }
static void guiLock(void) {}
static void guiUnlock(void) {}
static void lngFreeFromFile(char **strs) {} // the strings are not read from files here
static int lngLoadFromFile(char *path, char *name)
{
    loadedLng = path;
    return 1;
}

int listDir(char *path, const char *separator, int maxElem,
            int (*readEntry)(int index, const char *path, const char *separator, const char *name, unsigned char d_type))
{
    int i, j, index = 0;
    if (isGone(path))
        return 0;
    for (i = 0; i < nSimDirs; i++) {
        if (strcmp(simDirs[i].dir, path))
            continue;
        for (j = 0; j < 4 && simDirs[i].entries[j] && index < maxElem; j++)
            index = readEntry(index, path, separator, simDirs[i].entries[j], strstr(simDirs[i].entries[j], "thm_") ? DT_DIR : DT_REG);
    }
    return index;
}

int thmSetGuiValue(int themeID, int reload);
int thmGetGuiValue(void);
int thmFindGuiID(const char *theme);
char *thmGetFilePath(int themeID);
int lngSetGuiValue(int langID);
int lngGetGuiValue(void);
int lngFindGuiID(const char *lang);
char *lngGetFilePath(int langID);
const char **thmGetGuiList(void);
char **lngGetGuiList(void);

/* the state of src/themes.c */
static int guiThemeID = 0;
static int nThemes = 0;
static theme_file_t themes[THM_MAX_FILES];
static const char **guiThemesNames = NULL;
static int nGuiThemes = 0;
static int guiThemes[THM_MAX_FILES];
static int guiThemeCopy = -1;

/* the state of src/lang.c */
static int guiLangID = 0;
static char **lang_strs = internalEnglish;
static int nValidEntries = LANG_STR_COUNT;
static int nLanguages = 0;
static language_t languages[MAX_LANGUAGE_FILES];
static char **guiLangNames;
static int nGuiLanguages = 0;
static int guiLanguages[MAX_LANGUAGE_FILES];
static int guiLangCopy = -1;

int configGetDeviceRank(const char *path);
int listDeviceFiles(const device_file_t *files, int count, int *picked);
int dropDeviceFiles(device_file_t *files, int count, const char *prefix, int *inUse);
void thmRemoveElements(const char *path);
void lngRemoveLanguages(const char *path);
void sbDropThemesLanguages(const char *prefix, unsigned char *themesLoaded, unsigned char *languagesLoaded);

#include "thmlist_extracted.c"

/* --- the device code, as the tree has it --- */
static int usbThemesLoaded, usbLanguagesLoaded;
static unsigned char devThemesLoaded, devLanguagesLoaded;

/* A device page that lists its THM and LNG folders until found (bdmNeedsUpdate(), mmceNeedsUpdate()) */
static void devFound(const char *prefix, int mode)
{
    char path[64];

    nGone = 0;
    sprintf(path, "%sTHM", prefix);
    if (!devThemesLoaded && thmAddElements(path, "/", 1) > 0)
        devThemesLoaded = 1;
    sprintf(path, "%sLNG", prefix);
    if (!devLanguagesLoaded && lngAddLanguages(path, "/", mode) > 0)
        devLanguagesLoaded = 1;
}

/* The device's prefix changes in the settings (bdmUpdateDeviceData(), mmceSetPrefix()); drop says whether the
 * tree's code for that device drops the old prefix's themes and languages */
static void devPrefixChanged(const char *oldPrefix, int drop)
{
#ifdef HAVE_SB_DROP
    if (drop)
        sbDropThemesLanguages(oldPrefix, &devThemesLoaded, &devLanguagesLoaded);
#endif
}

static void start(void)
{
    int themeID = -1, langID = -1;

    lngAddLanguages("mc?:OPL", "/", -1);
    thmAddElements("mc?:OPL", "/", 0);
    if (cfgTheme)
        themeID = thmFindGuiID(cfgTheme);
    if (cfgLang)
        langID = lngFindGuiID(cfgLang);
    thmSetGuiValue(themeID, 0);
    lngSetGuiValue(langID);
}

static void usbFound(void)
{
    nGone = 0;
    if (!usbThemesLoaded && thmAddElements("mass0:THM", "/", 1) > 0)
        usbThemesLoaded = 1;
    if (!usbLanguagesLoaded && lngAddLanguages("mass0:LNG", "/", 0) > 0)
        usbLanguagesLoaded = 1;
}

static void usbGone(void)
{
    gone[nGone++] = "mass0:";
#ifdef HAVE_REMOVE
    thmRemoveElements("mass0:");
    lngRemoveLanguages("mass0:");
    usbThemesLoaded = 0;
    usbLanguagesLoaded = 0;
#endif
}

static void smbFound(void)
{
    nGone = 0;
    thmAddElements("smb0:THM", "\\", 1);
    lngAddLanguages("smb0:LNG", "\\", 2);
}

static void smbReconnect(void)
{
    gone[nGone++] = "smb0:";
#ifdef HAVE_REMOVE
    thmRemoveElements("smb0:");
    lngRemoveLanguages("smb0:");
#else
    thmReinit("smb0:");
#endif
}

static void printLoaded(const char *path, const char *prefix, const char *internal)
{
    const char *name, *colon;
    int len;

    if (path == NULL) {
        printf("%s", internal);
        return;
    }
    name = strstr(path, prefix) + strlen(prefix);
    len = (int)strcspn(name, "/\\.");
    colon = strchr(path, ':');
    printf("%.*s@%.*s%s", len, name, (int)(colon - path), path, isGone(path) ? "!" : "");
}

static void state(const char *step)
{
    int i;

    printf("%s thm[", step);
    for (i = 1; guiThemesNames[i] != NULL; i++)
        printf("%s%s%s", i > 1 ? " " : "", guiThemesNames[i], isGone(thmGetFilePath(i)) ? "!" : "");
    printf("]=");
    printLoaded(thmGetGuiValue() ? loadedThm : NULL, "thm_", "<OPL>");
    // the entry the menu marks as in use is not the theme loaded
    if (thmGetGuiValue() && strcmp(thmGetFilePath(thmGetGuiValue()), loadedThm))
        printf("(menu: %s)", guiThemesNames[thmGetGuiValue()]);
    printf(" lng[");
    for (i = 1; guiLangNames[i] != NULL; i++)
        printf("%s%s%s", i > 1 ? " " : "", guiLangNames[i], isGone(lngGetFilePath(i)) ? "!" : "");
    printf("]=");
    printLoaded(lngGetGuiValue() ? loadedLng : NULL, "lang_", "English");
    if (lngGetGuiValue() && strcmp(lngGetFilePath(lngGetGuiValue()), loadedLng))
        printf("(menu: %s)", guiLangNames[lngGetGuiValue()]);
    printf("; ");
}

static const char *entry(const char *name)
{
    return name != NULL ? name : "-";
}

static void addDir(const char *dir, const char *a, const char *b, const char *c)
{
    sim_dir_t *d = &simDirs[nSimDirs++];
    d->dir = dir;
    d->entries[0] = a;
    d->entries[1] = b;
    d->entries[2] = c;
    d->entries[3] = NULL;
}

int main(int argc, char **argv)
{
    const char *s = argc > 1 ? argv[1] : "";

    if (!strcmp(s, "dup-mc") || !strcmp(s, "dup-usb")) {
        // The same theme and language on the memory card and the USB drive
        home = !strcmp(s, "dup-mc") ? "mc0:" : "mass0:";
        cfgTheme = "A";
        cfgLang = "Russian";
        addDir("mc?:OPL", "thm_A", "lang_Russian.lng", NULL);
        addDir("mass0:THM", "thm_A", "thm_B", NULL);
        addDir("mass0:LNG", "lang_Russian.lng", NULL, NULL);
        start();
        state("start");
        usbFound();
        state("usb+");
    } else if (!strcmp(s, "remove")) {
        // The theme and language in use are only on the USB drive, which goes away and comes back
        cfgTheme = "B";
        cfgLang = "Russian";
        addDir("mc?:OPL", "thm_A", NULL, NULL);
        addDir("mass0:THM", "thm_B", NULL, NULL);
        addDir("mass0:LNG", "lang_Russian.lng", NULL, NULL);
        start();
        usbFound();
        state("usb+");
        usbGone();
        state("usb-");
        usbFound();
        state("usb+");
    } else if (!strcmp(s, "fallback")) {
        // Settings on the USB drive, the same theme and language on the memory card too; the drive goes away
        home = "mass0:";
        cfgTheme = "A";
        cfgLang = "Russian";
        addDir("mc?:OPL", "thm_A", "lang_Russian.lng", NULL);
        addDir("mass0:THM", "thm_A", NULL, NULL);
        addDir("mass0:LNG", "lang_Russian.lng", NULL, NULL);
        start();
        usbFound();
        state("usb+");
        usbGone();
        state("usb-");
    } else if (!strcmp(s, "shift")) {
        // The USB drive's theme and language are listed above the ones in use, from the SMB share; the drive goes away
        cfgTheme = "D";
        cfgLang = "German";
        addDir("mass0:THM", "thm_C", NULL, NULL);
        addDir("mass0:LNG", "lang_Russian.lng", NULL, NULL);
        addDir("smb0:THM", "thm_D", NULL, NULL);
        addDir("smb0:LNG", "lang_German.lng", NULL, NULL);
        start();
        usbFound();
        smbFound();
        state("smb+");
        usbGone();
        state("usb-");
    } else if (!strcmp(s, "smb")) {
        // The theme and language in use are on the memory card; the SMB share reconnects
        cfgTheme = "A";
        cfgLang = "Russian";
        addDir("mc?:OPL", "thm_A", "lang_Russian.lng", NULL);
        addDir("smb0:THM", "thm_C", NULL, NULL);
        addDir("smb0:LNG", "lang_German.lng", NULL, NULL);
        start();
        smbFound();
        state("smb+");
        smbReconnect();
        state("smb-");
        smbFound();
        state("smb+");
    } else if (!strcmp(s, "prefix-bdm") || !strcmp(s, "prefix-mmce")) {
        // The theme and language in use are in the folder the device's prefix is changed to: the BDM prefix
        // GAMES on a USB drive, or the MMCE slot 1 -> 2
        int bdm = !strcmp(s, "prefix-bdm");
        const char *oldPrefix = bdm ? "mass0:" : "mmce0:/";
        const char *newPrefix = bdm ? "mass0:GAMES/" : "mmce1:/";
        char thm[2][32], lng[32];
        int drop = 0;

#ifdef HAVE_BDM_PREFIX_DROP
        drop |= bdm;
#endif
#ifdef HAVE_MMCE_PREFIX_DROP
        drop |= !bdm;
#endif
        cfgTheme = "B";
        cfgLang = "German";
        sprintf(thm[0], "%sTHM", oldPrefix);
        sprintf(thm[1], "%sTHM", newPrefix);
        sprintf(lng, "%sLNG", newPrefix);
        addDir(thm[0], "thm_A", NULL, NULL);
        addDir(thm[1], "thm_B", NULL, NULL);
        addDir(lng, "lang_German.lng", NULL, NULL);
        start();
        devFound(oldPrefix, 0);
        state("dev+");
        devPrefixChanged(oldPrefix, drop);
        devFound(newPrefix, 0);
        state("prefix");
    } else if (!strcmp(s, "dialog")) {
        // Display Settings is open with the lists of the memory card; a USB drive with theme B and language German
        // comes, the user picks them, the drive goes, and the dialog's OK applies the picks
        const char **thm;
        char **lng;

        cfgTheme = "A";
        cfgLang = "Russian";
        addDir("mc?:OPL", "thm_A", "lang_Russian.lng", NULL);
        addDir("mass0:THM", "thm_B", NULL, NULL);
        addDir("mass0:LNG", "lang_German.lng", NULL, NULL);
        start();
        thm = thmGetGuiList();
        lng = lngGetGuiList();
        printf("open thm[%s] lng[%s]; ", entry(thm[1]), entry(lng[1]));
        fflush(stdout);
        usbFound();
        printf("usb+ thm[%s %s] lng[%s %s]; ", entry(thm[1]), entry(thm[2]), entry(lng[1]), entry(lng[2]));
        fflush(stdout);
        usbGone();
        printf("usb- thm[%s %s] lng[%s %s]; ", entry(thm[1]), entry(thm[2]), entry(lng[1]), entry(lng[2]));
        fflush(stdout);
        thmSetGuiValue(2, 0);
        lngSetGuiValue(2);
        state("OK");
    } else {
        printf("unknown scenario\n");
        return 2;
    }
    printf("\n");
    return 0;
}
