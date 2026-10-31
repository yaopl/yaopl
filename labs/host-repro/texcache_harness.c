/* Host harness for the art cache of src/texcache.c: cacheGetTexture() queues a load request that points at the
 * cache and one of its entries, and the IO thread runs cacheLoadImage() on it later. A theme change frees the
 * caches of the old theme (thmFree() -> cacheDestroyCache()) in between. Modes: texcache-t ok | destroyed |
 * blocked (the IO queue refuses the request, as while ioBlockOps() holds it). */
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

int guiFrameId = 100, guiInactiveFrames = 100;
static void *queued;
static int ioBlocked;
#define IO_CACHE_LOAD_ART 3
#define IO_ERR_IO_BLOCKED -2
static int ioPutRequest(int type, void *data)
{
    if (ioBlocked)
        return IO_ERR_IO_BLOCKED;
    queued = data;
    return 0;
}
static void ioRegisterHandler(int t, void *h) {}
static void guiLock(void) {}
static void guiUnlock(void) {}
static void rmUnloadTexture(GSTEXTURE *t) {}
static void texFree(GSTEXTURE *t)
{
    free(t->Mem);
    t->Mem = NULL;
    free(t->Clut);
    t->Clut = NULL;
}
static int getImage(item_list_t *l, char *folder, int rel, char *value, char *suffix, GSTEXTURE *tex, short psm)
{
    tex->Mem = malloc(64);
    tex->Clut = NULL;
    printf("loaded %s/%s_%s | ", folder ? folder : "", value, suffix);
    return 0;
}

#include "texcache_extracted.c"

int main(int argc, char **argv)
{
    const char *mode = argc > 1 ? argv[1] : "";
    item_list_t list = {0, &getImage};
    int cacheId = -1, uid = -1;
    image_cache_t *cache = cacheInitCache(0, "ART", 1, "COV", 2);

    if (!strcmp(mode, "ok")) {
        cacheGetTexture(cache, &list, &cacheId, &uid, "SLUS_123.45");
        cacheLoadImage(queued);
        printf("cover %s\n", cacheGetTexture(cache, &list, &cacheId, &uid, "SLUS_123.45") ? "shown" : "not shown");
        cacheDestroyCache(cache);
        return 0;
    }
    if (!strcmp(mode, "destroyed")) {
        cacheGetTexture(cache, &list, &cacheId, &uid, "SLUS_123.45");
        // a theme change frees the old theme's caches; the IO thread then runs the request it already had
        cacheDestroyCache(cache);
        fflush(stdout);
        cacheLoadImage(queued);
        printf("request on the freed cache: done\n");
        return 0;
    }
    if (!strcmp(mode, "blocked")) {
        ioBlocked = 1;
        cacheGetTexture(cache, &list, &cacheId, &uid, "SLUS_123.45");
        ioBlocked = 0;
        // the next frames ask again
        cacheGetTexture(cache, &list, &cacheId, &uid, "SLUS_123.45");
        if (queued)
            cacheLoadImage(queued);
        printf("cover %s\n", cacheGetTexture(cache, &list, &cacheId, &uid, "SLUS_123.45") ? "shown" : "never loaded");
        cacheDestroyCache(cache);
        return 0;
    }
    return 2;
}
