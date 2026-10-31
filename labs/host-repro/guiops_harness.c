/* Host harness for src/gui.c guiDeferUpdate() and guiHandleDeferredOps(): the list of operations the GUI
 * thread runs in its frame. The IO thread (priority 30) waiting in guiDeferUpdate() runs as soon as the GUI
 * thread (priority 31) gives the list's semaphore back; the stub SignalSema() runs that IO-thread step at that
 * point, then the IO thread appends once more before the next frame (as deferredInit() does). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)

struct gui_update_t
{
    int type;
};
struct gui_update_list_t
{
    struct gui_update_t *item;
    struct gui_update_list_t *next;
};
static struct gui_update_list_t *gUpdateList;
static struct gui_update_list_t *gUpdateEnd;
static int gSemaId, gScheduledOps, gCompletedOps;
static int inDrain, preempted, handled;
static struct gui_update_t ops[3];
int guiDeferUpdate(struct gui_update_t *op);

static void WaitSema(int id) {}
static void SignalSema(int id)
{
    if (inDrain && !preempted) {
        preempted = 1;
        guiDeferUpdate(&ops[1]); // IO thread, woken by this signal
    }
}
static void guiHandleOp(struct gui_update_t *item) { handled++; }

#include "guiops_extracted.c"

int main(void)
{
    guiDeferUpdate(&ops[0]);
    inDrain = 1;
    guiHandleDeferredOps(); // GUI thread, in its frame
    inDrain = 0;
    printf("after the drain: list %s, end %s\n", gUpdateList ? "not empty" : "empty", gUpdateEnd ? "set" : "NULL");
    guiDeferUpdate(&ops[2]); // IO thread again, before the next frame
    guiHandleDeferredOps();
    printf("next append ok, %d of 3 operations run\n", handled);
    return 0;
}
