/* Host harness for src/lang.c lngSetGuiValue() with lngLoadFromFile(): switching between a language file
 * and the built-in English. The stubs count the allocations; the strings read from the file must be freed
 * when another language, or English, replaces them. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)
#define O_RDONLY       0
#define LANG_STR_COUNT 4

static int liveAllocs;
static void *harness_malloc(size_t n)
{
    liveAllocs++;
    return malloc(n);
}
static void *harness_calloc(size_t n, size_t s)
{
    liveAllocs++;
    return calloc(n, s);
}
static void harness_free(void *p)
{
    if (p)
        liveAllocs--;
    free(p);
}

static char *internalEnglish[LANG_STR_COUNT] = {"English", "Back", "Accept", "Settings"};
typedef struct
{
    char *filePath;
    char *name;
} language_t;
typedef struct
{
    int line;
} file_buffer_t;
static file_buffer_t fileBuffer;
static file_buffer_t *openFileBuffer(char *path, int mode, short allocResult, unsigned int size)
{
    fileBuffer.line = 0;
    return &fileBuffer;
}
static int readFileBuffer(file_buffer_t *f, char **outBuf)
{
    if (f->line >= LANG_STR_COUNT)
        return 0;
    *outBuf = harness_malloc(16);
    snprintf(*outBuf, 16, "line %d", f->line++);
    return 1;
}
static int closeFileBuffer(file_buffer_t *f) { return 0; }
static int fntLoadDefault(char *path) { return -1; }
static int thmGetGuiValue(void) { return 0; }
static int thmSetGuiValue(int id, int reload) { return 0; }
static void bgmMute(void) {}
static void bgmUnMute(void) {}
static void guiLock(void) {}
static void guiUnlock(void) {}

static int guiLangID = 0;
static char **lang_strs = internalEnglish;
static int nValidEntries = LANG_STR_COUNT;
static language_t languages[1] = {{"mass0:LNG/lang_Russian.lng", "Russian"}};
static int guiLanguages[1] = {0};
static int guiLangCopy = -1;

#define malloc harness_malloc
#define calloc harness_calloc
#define free   harness_free
#include "langswitch_extracted.c"
#undef malloc
#undef calloc
#undef free

int main(void)
{
    int i;

    for (i = 0; i < 3; i++) {
        lngSetGuiValue(1); // the language file
        lngSetGuiValue(0); // English
    }
    printf("3 times file -> English: %d allocations left, strings %s\n", liveAllocs, lang_strs == internalEnglish ? "English" : "of the file");
    return 0;
}
