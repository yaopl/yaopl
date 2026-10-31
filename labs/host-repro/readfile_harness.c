/* Host harness for src/util.c readFile(): the whole-file read used for fonts and theme files.
 * Argument: "ok", "nomem" (the buffer cannot be allocated) or "short" (the device goes away during the
 * read: read() returns less than the file size). The stubs count open descriptors. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOG(...)
#define O_RDONLY  0
#define FILE_SIZE 4096

static int openFds, failAlloc, shortRead;
static void *harness_malloc(size_t n) { return failAlloc ? NULL : malloc(n); }
#define memalign(a, n) harness_malloc(n)
static int openFile(char *path, int mode)
{
    openFds++;
    return 3;
}
static int getFileSize(int fd) { return FILE_SIZE; }
static int close(int fd)
{
    openFds--;
    return 0;
}
static int read(int fd, void *buf, unsigned int n)
{
    memset(buf, 'x', shortRead ? n / 2 : n);
    return shortRead ? (int)(n / 2) : (int)n;
}

#define malloc harness_malloc
#include "readfile_extracted.c"
#undef malloc

int main(int argc, char **argv)
{
    const char *c = argc > 1 ? argv[1] : "ok";
    int size = -1;
    void *buf;

    failAlloc = !strcmp(c, "nomem");
    shortRead = !strcmp(c, "short");
    buf = readFile("mass0:THM/thm_x/font.ttf", -1, &size);
    printf("%s: buffer %s, size %d, descriptors left open %d\n", c, buf ? "returned" : "NULL", size, openFds);
    free(buf);
    return 0;
}
