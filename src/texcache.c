#include "include/opl.h"
#include "include/texcache.h"
#include "include/textures.h"
#include "include/ioman.h"
#include "include/gui.h"
#include "include/util.h"
#include "include/renderman.h"

typedef struct
{
    image_cache_t *cache;
    cache_entry_t *entry;
    item_list_t *list;
    // only for comparison if the deferred action is still valid
    int cacheUID;
    char *value;
} load_image_request_t;

// Io handled action...
// The cache may be freed while the request waits or loads: a theme change frees the old theme's caches
// (cacheDestroyCache(), under guiLock()) and sets req->cache to NULL. So the cache is read and written only under
// guiLock(), and the image is loaded into a texture of the request's own.
static void cacheLoadImage(void *data)
{
    load_image_request_t *req = data;
    char prefix[256], suffix[32], *folder = NULL;
    int isPrefixRelative, result;
    GSTEXTURE texture;

    // Safeguards...
    if (!req)
        return;

    item_list_t *handler = req->list;

    guiLock();
    // the cache is gone, or its entry was already reused
    if (!handler || !req->cache || req->cacheUID != req->entry->UID) {
        guiUnlock();
        free(req);
        return;
    }
    if (req->cache->prefix) {
        snprintf(prefix, sizeof(prefix), "%s", req->cache->prefix);
        folder = prefix;
    }
    snprintf(suffix, sizeof(suffix), "%s", req->cache->suffix);
    isPrefixRelative = req->cache->isPrefixRelative;
    guiUnlock();

    memset(&texture, 0, sizeof(texture));
    result = handler->itemGetImage(handler, folder, isPrefixRelative, req->value, suffix, &texture, GS_PSM_CT24);

    guiLock();
    if (req->cache && req->cacheUID == req->entry->UID) {
        // seems okay. we can proceed
        texFree(&req->entry->texture);
        req->entry->texture = texture;
        if (result < 0)
            req->entry->lastUsed = 0;
        else
            req->entry->lastUsed = guiFrameId;
        req->entry->qr = NULL;
    } else
        texFree(&texture);
    guiUnlock();

    free(req);
}

void cacheInit()
{
    ioRegisterHandler(IO_CACHE_LOAD_ART, &cacheLoadImage);
}

void cacheEnd()
{
    // nothing to do... others have to destroy the cache via cacheDestroyCache
}

static void cacheClearItem(cache_entry_t *item, int freeTxt)
{
    if (freeTxt && item->texture.Mem) {
        rmUnloadTexture(&item->texture);
        free(item->texture.Mem);
        if (item->texture.Clut)
            free(item->texture.Clut);
    }

    memset(item, 0, sizeof(cache_entry_t));
    item->texture.Mem = NULL;
    item->texture.Vram = 0;
    item->texture.Clut = NULL;
    item->texture.VramClut = 0;
    item->texture.ClutStorageMode = GS_CLUT_STORAGE_CSM1; // Default
    item->qr = NULL;
    item->lastUsed = -1;
    item->UID = 0;
}

image_cache_t *cacheInitCache(int userId, const char *prefix, int isPrefixRelative, const char *suffix, int count)
{
    image_cache_t *cache = (image_cache_t *)malloc(sizeof(image_cache_t));
    cache->userId = userId;
    cache->count = count;
    cache->prefix = NULL;
    int length;
    if (prefix) {
        length = strlen(prefix) + 1;
        cache->prefix = (char *)malloc(length * sizeof(char));
        memcpy(cache->prefix, prefix, length);
    }
    cache->isPrefixRelative = isPrefixRelative;
    length = strlen(suffix) + 1;
    cache->suffix = (char *)malloc(length * sizeof(char));
    memcpy(cache->suffix, suffix, length);
    cache->nextUID = 1;
    cache->content = (cache_entry_t *)malloc(count * sizeof(cache_entry_t));

    int i;
    for (i = 0; i < count; ++i)
        cacheClearItem(&cache->content[i], 0);

    return cache;
}

void cacheDestroyCache(image_cache_t *cache)
{
    int i;
    for (i = 0; i < cache->count; ++i) {
        // A request still queued or loading is the IO thread's: it frees it, seeing the cache is gone
        if (cache->content[i].qr)
            ((load_image_request_t *)cache->content[i].qr)->cache = NULL;
        cacheClearItem(&cache->content[i], 1);
    }

    free(cache->prefix);
    free(cache->suffix);
    free(cache->content);
    free(cache);
}

GSTEXTURE *cacheGetTexture(image_cache_t *cache, item_list_t *list, int *cacheId, int *UID, char *value)
{
    if (*cacheId == -2) {
        return NULL;
    } else if (*cacheId != -1) {
        cache_entry_t *entry = &cache->content[*cacheId];
        if (entry->UID == *UID) {
            if (entry->qr)
                return NULL;
            else if (entry->lastUsed == 0) {
                *cacheId = -2;
                return NULL;
            } else {
                entry->lastUsed = guiFrameId;
                return &entry->texture;
            }
        }

        *cacheId = -1;
    }

    // under the cache pre-delay (to avoid filling cache while moving around)
    if (guiInactiveFrames < list->delay)
        return NULL;

    cache_entry_t *currEntry, *oldestEntry = NULL;
    int i, rtime = guiFrameId;

    for (i = 0; i < cache->count; i++) {
        currEntry = &cache->content[i];
        if ((!currEntry->qr) && (currEntry->lastUsed < rtime)) {
            oldestEntry = currEntry;
            rtime = currEntry->lastUsed;
            *cacheId = i;
        }
    }

    if (oldestEntry) {
        load_image_request_t *req = malloc(sizeof(load_image_request_t) + strlen(value) + 1);
        req->cache = cache;
        req->entry = oldestEntry;
        req->list = list;
        req->value = (char *)req + sizeof(load_image_request_t);
        strcpy(req->value, value);
        req->cacheUID = cache->nextUID;

        cacheClearItem(oldestEntry, 1);
        oldestEntry->qr = req;
        oldestEntry->UID = cache->nextUID;

        *UID = cache->nextUID++;

        // Refused while the IO thread is blocked: forget the request, so the image is asked for again later
        if (ioPutRequest(IO_CACHE_LOAD_ART, req) != 0) {
            cacheClearItem(oldestEntry, 0);
            *cacheId = -1;
            free(req);
        }
    }

    return NULL;
}
