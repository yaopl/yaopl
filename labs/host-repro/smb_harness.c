/* Host harness for the read and write loops of the in-game SMB client (modules/iopcore/cdvdman/smb.c
 * smb_ReadAndX(), smb_ReadFile(), smb_WriteAndX(), smb_WriteFile()) over a scripted transport:
 *   smb read ok      the reply comes in three fragments
 *   smb read error   the receive fails at once
 *   smb read closed  the server closes the connection after the first fragment
 *   smb write ok | smb write error
 * Prints the result, whether the data arrived, and whether smb_io_sema is free again: a taken
 * semaphore blocks the next disc read, VMC access and IGR (DeviceLock()) for ever. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed short s16;
typedef long long s64;
typedef void *OplSmbPwHashFunc_t; /* oplsmb.h, only named in a prototype */
#define __attribute__(x)
#pragma pack(push, 1)
#include "smb.h"

/* the client's message buffer, as declared in smb.c */
static struct
{
    u32 sessionHeader;
    union
    {
        u8 u8buff[MAX_SMB_BUF + MAX_SMB_BUF_HDR];
        ReadAndXRequest_t readAndXRequest;
        ReadAndXResponse_t readAndXResponse;
        WriteAndXRequest_t writeAndXRequest;
        WriteAndXResponse_t writeAndXResponse;
    } smb;
} SMB_buf;
#pragma pack(pop)

struct sockaddr;
typedef int socklen_t;

static u16 UID, TID;
static int main_socket = 3;
static int smb_io_sema = 1;
static int sema_count = 1;
static void WaitSema(int sema) { sema_count--; }
static void SignalSema(int sema) { sema_count++; }

#define DATA_OFFSET 64 /* SMB header and ReadAndX words, then padding up to the data */
#define FRAGMENT    1000
static const char *scenario;
static int payload_sent, payload_total;

static void nb_SetSessionMessage(u32 size) { SMB_buf.sessionHeader = ((size & 0xff0000) >> 8) | ((size & 0xff00) << 8) | ((size & 0xff) << 24); }
static int nb_GetSessionMessageLength(void) { return (int)(((SMB_buf.sessionHeader << 8) & 0xff0000) | ((SMB_buf.sessionHeader >> 8) & 0xff00) | ((SMB_buf.sessionHeader >> 24) & 0xff)); }
static u8 nb_GetPacketType(void) { return (u8)(SMB_buf.sessionHeader & 0xff); }

static int SendData(int sock, char *buf, int size) { return size; }

/* WriteAndX: the whole exchange */
static int GetSMBServerReply(int shdrlen, void *spayload, int rhdrlen)
{
    if (!strcmp(scenario, "error"))
        return -1;
    memset(&SMB_buf.smb.writeAndXResponse, 0, sizeof(WriteAndXResponse_t));
    SMB_buf.smb.writeAndXResponse.smbH.Magic = SMB_MAGIC;
    return (int)sizeof(WriteAndXResponse_t);
}

/* The custom lwip_recvfrom() of SMSTCPIP: the first call copies hlen header bytes to mem and the rest of the
   segment to payload; it returns the bytes copied in all, 0 once the connection is closed, -1 on an error. */
static int fake_recvfrom(int s, void *mem, int hlen, void *payload, int plen, unsigned int flags, struct sockaddr *from, socklen_t *fromlen)
{
    int n;

    if (mem != NULL) {
        if (!strcmp(scenario, "error"))
            return -1;
        nb_SetSessionMessage(DATA_OFFSET + payload_total);
        SMB_buf.smb.readAndXResponse.smbH.Magic = SMB_MAGIC;
        SMB_buf.smb.readAndXResponse.DataOffset = DATA_OFFSET;
        SMB_buf.smb.readAndXResponse.DataLengthLow = (u16)payload_total;
        SMB_buf.smb.readAndXResponse.DataLengthHigh = 0;
        n = FRAGMENT;
        memset(payload, 0xA5, n);
        payload_sent = n;
        return 4 + DATA_OFFSET + n;
    }
    if (!strcmp(scenario, "closed"))
        return 0;
    n = payload_total - payload_sent < FRAGMENT ? payload_total - payload_sent : FRAGMENT;
    if (n > plen)
        n = plen;
    memset(payload, 0xA5, n);
    payload_sent += n;
    return n;
}
static int (*plwip_recvfrom)(int s, void *mem, int hlen, void *payload, int plen, unsigned int flags, struct sockaddr *from, socklen_t *fromlen) = fake_recvfrom;

#include "smb_extracted.c"

int main(int argc, char **argv)
{
    static u8 buf[2500];
    int r, i, good;

    if (argc < 3)
        return 2;
    scenario = argv[2];
    payload_total = sizeof(buf);
    memset(buf, 0, sizeof(buf));
    if (!strcmp(argv[1], "read")) {
        r = smb_ReadFile(1, 0, 0, buf, sizeof(buf));
        for (i = 0, good = 0; i < (int)sizeof(buf); i++)
            good += buf[i] == 0xA5;
        printf("smb_ReadFile = %d, %d of %d bytes read, smb_io_sema %s\n", r, good, (int)sizeof(buf), sema_count == 1 ? "free" : "LEFT TAKEN");
    } else {
        r = smb_WriteFile(1, 0, 0, buf, sizeof(buf));
        printf("smb_WriteFile = %d, smb_io_sema %s\n", r, sema_count == 1 ? "free" : "LEFT TAKEN");
    }
    return 0;
}
