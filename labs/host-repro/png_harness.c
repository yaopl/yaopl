/* Host harness for src/textures.c texLoadAll() with libpng 1.6.53 and zlib 1.3.1 (run_all.ps1 fetches
 * and builds them).
 *   png-t load <file.png>
 *       loads the PNG as a theme image or a cover; prints the result, the texture, the allocations of
 *       OPL's code still live (after a failure: what the caller never gets back; after a success: what
 *       is left once texFree() ran) and the largest block libpng allocated
 *   png-t oom <file.png> <n>
 *       the same, with the n-th allocation of OPL's code failing (1 is the file buffer)
 *   png-t race <file.png> <other.png>
 *       loads the first PNG; between reading its palette and converting its pixels the other PNG is
 *       loaded, as the IO thread does when it preempts the GUI thread in the middle of a theme load */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <png.h>

typedef unsigned char u8;
typedef unsigned int u32;
#define LOG(...)
#undef O_RDONLY
#define O_RDONLY (_O_RDONLY | _O_BINARY)

// as gsKit's GSTEXTURE and its constants
typedef struct
{
    u32 Width, Height;
    u8 PSM, ClutPSM;
    u32 TBW;
    void *Mem;
    u32 *Clut;
    u32 Vram, VramClut;
    u32 Filter;
    u8 ClutStorageMode;
    u8 Delayed;
} GSTEXTURE;
#define GS_PSM_CT32          0x00
#define GS_PSM_CT24          0x01
#define GS_PSM_T8            0x13
#define GS_PSM_T4            0x14
#define GS_FILTER_LINEAR     0x01
#define GS_CLUT_STORAGE_CSM1 0x00
static u32 gsKit_texture_size_ee(int width, int height, int psm)
{
    switch (psm) {
        case GS_PSM_CT32:
            return width * height * 4;
        case GS_PSM_CT24:
            return width * height * 3;
        case GS_PSM_T8:
            return width * height;
        default:
            return width * height / 2;
    }
}
// VRAM is allocated in 8 KiB pages of 64x32 32-bit pixels
static u32 gsKit_texture_size(int width, int height, int psm)
{
    return (gsKit_texture_size_ee((width + 63) & ~63, (height + 31) & ~31, psm) + 8191) & ~8191;
}

// as include/textures.h
#define ERR_BAD_FILE      -1
#define ERR_READ_STRUCT   -2
#define ERR_INFO_STRUCT   -3
#define ERR_SET_JMP       -4
#define ERR_BAD_DIMENSION -5
#define ERR_MISSING_ALPHA -6
#define ERR_BAD_DEPTH     -7

// as src/textures.c: no built-in textures here
typedef struct
{
    int id;
    char *name;
    void **texture;
} texture_t;
static texture_t internalDefault[1];

#define FILL 0xA5 // what a new block holds: a texture still full of it was never written
static int liveAllocs, allocNo, failAt;
// The plain build keeps a zeroed tail behind every block, so that a read past the end finds the same
// bytes on every machine (the ASan build reports the read)
#ifdef __SANITIZE_ADDRESS__
#define TAIL 0
#else
#define TAIL 65536
#endif
static void *harness_malloc(size_t n)
{
    size_t *p;
    if (++allocNo == failAt)
        return NULL;
    liveAllocs++;
    p = malloc(n + 16 + TAIL);
    *p = n;
    memset((char *)p + 16, FILL, n);
    memset((char *)p + 16 + n, 0, TAIL);
    return (char *)p + 16;
}
static void *harness_calloc(size_t count, size_t n)
{
    void *p = harness_malloc(count * n);
    if (p)
        memset(p, 0, count * n);
    return p;
}
static void *harness_memalign(size_t align, size_t n)
{
    return harness_malloc(n);
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

// libpng's own allocations: only the largest one is of interest
static size_t pngLargest;
static png_voidp harness_png_malloc(png_structp pngPtr, png_alloc_size_t n)
{
    if (n > pngLargest)
        pngLargest = n;
    return malloc(n);
}
static void harness_png_free(png_structp pngPtr, png_voidp p)
{
    free(p);
}

static int texLoadAll(GSTEXTURE *texture, const char *filePath, int texId);
void texFree(GSTEXTURE *texture);
static const char *raceFile;
// png_read_image() of the load under test: the other thread's load runs first
static void harness_read_image(png_structp pngPtr, png_bytepp rows)
{
    if (raceFile) {
        GSTEXTURE other;
        const char *file = raceFile;
        raceFile = NULL;
        printf("other load %d\n", texLoadAll(&other, file, -1));
        texFree(&other);
    }
    png_read_image(pngPtr, rows);
}

#define malloc         harness_malloc
#define calloc         harness_calloc
#define memalign       harness_memalign
#define free           harness_free
#define png_read_image harness_read_image
#define png_create_read_struct(ver, errPtr, errFn, warnFn) \
    png_create_read_struct_2(ver, errPtr, errFn, warnFn, NULL, harness_png_malloc, harness_png_free)
#include "png_extracted.c"
#undef malloc
#undef calloc
#undef memalign
#undef free
#undef png_read_image

static const char *psmName(int psm)
{
    switch (psm) {
        case GS_PSM_CT32:
            return "CT32";
        case GS_PSM_CT24:
            return "CT24";
        case GS_PSM_T8:
            return "T8";
        default:
            return "T4";
    }
}

int main(int argc, char **argv)
{
    GSTEXTURE tex;
    int ret, unwritten, i;
    size_t size;

    if (argc < 3)
        return 2;
    if (!strcmp(argv[1], "oom") && argc > 3)
        failAt = atoi(argv[3]);
    if (!strcmp(argv[1], "race") && argc > 3)
        raceFile = argv[3];

    memset(&tex, 0, sizeof(tex));
    ret = texLoadAll(&tex, argv[2], -1);
    printf("result %d, %ux%u %s\n", ret, tex.Width, tex.Height, psmName(tex.PSM));

    if (tex.Mem) {
        size = gsKit_texture_size_ee(tex.Width, tex.Height, tex.PSM);
        for (unwritten = 1, i = 0; i < (int)size && unwritten; i++)
            unwritten = ((u8 *)tex.Mem)[i] == FILL;
        printf("pixels %s", unwritten ? "never written" : "read");
        if (tex.Clut)
            printf(", palette entry 1 %08x", tex.Clut[1]);
        printf("\n");
    } else
        printf("no pixels%s\n", tex.Clut ? ", a palette" : "");

    if (ret >= 0)
        texFree(&tex);
    printf("OPL blocks left %d, libpng's largest block %u KiB\n", liveAllocs, (unsigned)((pngLargest + 1023) / 1024));
    return 0;
}
