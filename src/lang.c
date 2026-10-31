#include "include/opl.h"
#include "include/lang.h"
#include "include/util.h"
#include "include/fntsys.h"
#include "include/ioman.h"
#include "include/themes.h"
#include "include/sound.h"
#include "include/gui.h"

static int guiLangID = 0;
static char **lang_strs = internalEnglish;
static int nValidEntries = LANG_STR_COUNT;

static int nLanguages = 0;
static language_t languages[MAX_LANGUAGE_FILES];
static char **guiLangNames;
static int nGuiLanguages = 0;
static int guiLanguages[MAX_LANGUAGE_FILES]; // the copy in languages[] listed for each name, by GUI ID - 1
static int guiLangCopy = -1;                 // the copy in languages[] that is loaded, -1 for internal English

// localised string getter
char *_l(unsigned int id)
{
    return lang_strs[id];
}

static void lngFreeFromFile(char **lang_strs)
{
    if (guiLangID == 0)
        return;

    int strId = 0;
    for (; strId < nValidEntries; strId++) {
        free(lang_strs[strId]);
        lang_strs[strId] = NULL;
    }
    free(lang_strs);
}

static int lngLoadFont(const char *dir, const char *name)
{
    char path[256];

    snprintf(path, sizeof(path), "%sfont_%s.ttf", dir, name);
    LOG("LANG Custom TTF font path: %s\n", path);
    if (fntLoadDefault(path) == 0)
        return 0;

    snprintf(path, sizeof(path), "%sfont_%s.otf", dir, name);
    LOG("LANG Custom OTF font path: %s\n", path);
    if (fntLoadDefault(path) == 0)
        return 0;

    LOG("LANG cannot load font.\n");

    return -1;
}

static int lngLoadFromFile(char *path, char *name)
{
    char dir[128];

    file_buffer_t *fileBuffer = openFileBuffer(path, O_RDONLY, 1, 1024);
    if (fileBuffer) {
        // file exists, try to read it and load the custom lang
        char **curL = lang_strs;
        char **newL = (char **)calloc(LANG_STR_COUNT, sizeof(char *));

        int strId = 0;
        while (strId < LANG_STR_COUNT && readFileBuffer(fileBuffer, &newL[strId])) {
            strId++;
        }
        closeFileBuffer(fileBuffer);

        LOG("LANG Loaded %d entries\n", strId);

        int newEntries = strId;
        // if necessary complete lang with default internal
        while (strId < LANG_STR_COUNT) {
            LOG("LANG Default entry added: %s\n", internalEnglish[strId]);
            newL[strId] = internalEnglish[strId];
            strId++;
        }
        // lngAddLanguages() runs in the IO thread when a device with languages is found: free the old strings
        // only while the GUI thread is not drawing them.
        guiLock();
        lang_strs = newL;
        lngFreeFromFile(curL);
        guiUnlock();

        // remember how many entries were read from the file (for the free later)
        nValidEntries = newEntries;

        int len = strlen(path) - strlen(name) - 9; // -4 for extension,  -5 for prefix
        memcpy(dir, path, len);
        dir[len] = '\0';

        lngLoadFont(dir, name);

        return 1;
    }
    return 0;
}

char *lngGetValue(void)
{
    return guiLangNames[guiLangID];
}

static int lngReadEntry(int index, const char *path, const char *separator, const char *name, unsigned char d_type)
{
    if (d_type != DT_DIR) {
        if (strstr(name, ".lng") || strstr(name, ".LNG")) {
            // The language name is cut from lang_<Name>.lng: a shorter file name has none, and gave
            // a negative length below
            if (strlen(name) < 10)
                return index;

            language_t *currLang = &languages[nLanguages + index];

            // filepath for this language file
            int length = strlen(path) + 1 + strlen(name) + 1;
            currLang->filePath = (char *)malloc(length * sizeof(char));
            sprintf(currLang->filePath, "%s%s%s", path, separator, name);

            // extract name for this language (will be used for the English translation)
            length = strlen(name) - 5 - 4 + 1;
            currLang->name = (char *)malloc(length * sizeof(char));
            memcpy(currLang->name, name + 5, length);
            currLang->name[length - 1] = '\0';

            /*file_buffer_t* fileBuffer = openFileBuffer(currLang->filePath, 1, 1024);
            if (fileBuffer) {
                // read localized name of language from file
                if (readLineContext(fileBuffer, &currLang->name))
                    readLineContext(fileBuffer, &currLang->fontName);
                closeFileBuffer(fileBuffer);
            }*/

            index++;
        }
    }
    return index;
}

static void lngRebuildLangNames(void)
{
    int i;

    if (guiLangNames)
        free(guiLangNames);

    // build the languages name list
    guiLangNames = (char **)malloc((nLanguages + 2) * sizeof(char **));

    // add default internal (english)
    guiLangNames[0] = internalEnglish[0];

    // A language found on several devices is listed once, where it was first found, with its copy of the lowest rank
    nGuiLanguages = listDeviceFiles(languages, nLanguages, guiLanguages);
    for (i = 0; i < nGuiLanguages; i++)
        guiLangNames[i + 1] = languages[guiLanguages[i]].name;

    guiLangNames[nGuiLanguages + 1] = NULL;
}

int lngAddLanguages(char *path, const char *separator, int mode)
{
    int result;

    result = listDir(path, separator, MAX_LANGUAGE_FILES - nLanguages, &lngReadEntry);
    nLanguages += result;
    lngRebuildLangNames();

    const char *temp;
    if (configGetStr(configGetByType(CONFIG_OPL), "language_text", &temp)) {
        if (lngSetGuiValue(lngFindGuiID(temp)))
            moduleUpdateMenu(mode, 0, 1);
    }

    return result;
}

// Drops the languages of a device that is gone. The language in use moves to another copy of it, or to English until
// the device is back: the settings keep its name.
void lngRemoveLanguages(const char *path)
{
    char name[256] = "";
    int i, j;

    if (path[0] == '\0')
        return;

    if (guiLangCopy >= 0)
        snprintf(name, sizeof(name), "%s", languages[guiLangCopy].name);

    // guiLangCopy becomes -2 if the language in use is dropped: loaded, but its file is gone
    j = dropDeviceFiles(languages, nLanguages, path, &guiLangCopy);
    if (j == nLanguages)
        return;

    nLanguages = j;
    lngRebuildLangNames();

    if (name[0] != '\0') {
        int lost = (guiLangCopy == -2);

        // Keeps the copy in use, or loads another one or English (then it returns 0)
        lngSetGuiValue(lngFindGuiID(name));
        if (lost) {
            for (i = 0; i < MODE_COUNT; i++)
                moduleUpdateMenu(i, 0, 1);
        }
    }
}

void lngInit(void)
{
    fntInit();

    lngAddLanguages(gBaseMCDir, "/", -1);
}

void lngEnd(void)
{
    lngFreeFromFile(lang_strs);

    int i = 0;
    for (; i < nLanguages; i++) {
        free(languages[i].name);
        free(languages[i].filePath);
    }

    free(guiLangNames);

    fntEnd();
}

int lngSetGuiValue(int langID)
{
    if (langID != -1) {
        int copy = langID != 0 ? guiLanguages[langID - 1] : -1;

        if (guiLangCopy != copy) {
            bgmMute();
            if (copy != -1) {
                language_t *currLang = &languages[copy];
                if (lngLoadFromFile(currLang->filePath, currLang->name)) {
                    guiLangID = langID;
                    guiLangCopy = copy;
                    thmSetGuiValue(thmGetGuiValue(), 1);
                    bgmUnMute();
                    return 1;
                }
            }
            // The strings of a language file go with it (lngFreeFromFile() keeps internal English). The IO thread
            // gets here too, when the language's file goes away: not while the GUI thread draws them.
            guiLock();
            lngFreeFromFile(lang_strs);
            lang_strs = internalEnglish;
            guiUnlock();
            guiLangID = 0;
            guiLangCopy = -1;
            // lang switched back to internalEnglish, reload default font
            fntLoadDefault(NULL);
            thmSetGuiValue(thmGetGuiValue(), 1);
            bgmUnMute();
        } else
            guiLangID = langID;
    }
    return 0;
}

int lngGetGuiValue(void)
{
    return guiLangID;
}

int lngFindGuiID(const char *lang)
{
    if (lang) {
        int i = 0;
        for (; i < nGuiLanguages; i++) {
            if (strcasecmp(languages[guiLanguages[i]].name, lang) == 0)
                return i + 1; // shift for Gui id
        }
    }
    return 0;
}

char **lngGetGuiList(void)
{
    return guiLangNames;
}

char *lngGetFilePath(int langID)
{
    // The language notification can still be shown after its device went away
    if (langID < 1 || langID > nGuiLanguages)
        return "";

    language_t *currLang = &languages[guiLanguages[langID - 1]];
    char *path = currLang->filePath;

    return path;
}
