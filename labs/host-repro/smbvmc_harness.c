/* Host harness for the SMB VMC device of mcemu (modules/mcemu/device-smb.c DeviceWritePage(),
 * DeviceReadPage()) over scripted results of the SMB client:
 *   smbvmc <write result> <read result> <open result>
 * smb_WriteFile() and smb_ReadFile() return the byte count, or 0 or a negative code on an error;
 * smb_OpenAndX() returns 1 or a negative code. Prints what the device tells mcemu (1 = page done). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#define DPRINTF(...) ((void)0)

typedef struct
{
    u16 PageSize;
} McSpec;
typedef struct
{
    char fname[64];
    u16 fid;
    McSpec cspec;
} McImageSpec;
static McImageSpec vmcSpec[2];

static int write_result, read_result, open_result;
static int smb_WriteFile(u16 FID, u32 offsetlow, u32 offsethigh, void *writebuf, int nbytes) { return write_result; }
static int smb_ReadFile(u16 FID, u32 offsetlow, u32 offsethigh, void *readbuf, int nbytes) { return read_result; }
static int smb_OpenAndX(char *filename, u16 *FID, int Write)
{
    if (open_result > 0)
        *FID = 5;
    return open_result;
}

#include "smbvmc_extracted.c"

int main(int argc, char **argv)
{
    u8 page[512];
    int w, r, o;

    if (argc < 4)
        return 2;
    write_result = atoi(argv[1]);
    read_result = atoi(argv[2]);
    open_result = atoi(argv[3]);
    vmcSpec[0].cspec.PageSize = sizeof(page);
    vmcSpec[0].fid = 5;
    w = DeviceWritePage(0, page, 1);
    r = DeviceReadPage(0, page, 1);
    vmcSpec[0].fid = 0xFFFF; /* not open yet */
    o = DeviceReadPage(0, page, 1);
    printf("DeviceWritePage = %d, DeviceReadPage = %d, DeviceReadPage before open = %d\n", w, r, o);
    return 0;
}
