# oss-services/tftp-server

## Overview

This component is an extract of the [lwIP](https://savannah.nongnu.org/projects/lwip/) tftp server example application.  The example code included below can be used to weld the tftp server to the `wofs` write-once file system component.

## Required components

- oss-services/lwip

## Recommended components

- oss-services/wofs
- simple-services/syslog

## Integrate the source

- Copy the 'src' directory into an appropriate place in the host project

## Configure

- Some options can be configured by modifying `lwip/include/lwip/apps/tftp_opts.h` though this should not be necessary in most cases.

## Run

The following example code illustrates how to weld the `tftp-server` to the `wofs` file system.  It's assumed a functional lwIP stack has already been started.

```C
/* LwIP TFTP server includes */
#include "tftp_server.h"
#include "romfs.h"
#include "romfs_devman.h"
/***********************************************************************
 * TFTP Server
 **********************************************************************/
struct tftp_handle {
    int fd;
    int totalLength;
};

void *tftpOpen(const char* fname, const char* mode, u8_t write);
void tftpClose(void* handle);
int tftpRead(void* handle, void* buf, int bytes);
int tftpWrite(void* handle, struct pbuf* p);

struct tftp_context tftp = {
    .open = tftpOpen,
    .close = tftpClose,
    .read = tftpRead,
    .write = tftpWrite
};

struct tftp_handle tftpHandle;

void *tftpOpen(const char* fname, const char* mode, u8_t write)
{
    int flags;
    int fd;

    syslog_printf("tftp open: \"%s\", mode: \"%s\", rw: %s",
        fname, mode, write == 0 ? "read" : "write");

    if (write == 0) {
        flags = O_RDONLY;
    } else {
        flags = O_WRONLY | O_TRUNC | O_CREAT;
    }

    tftpHandle.totalLength = 0;
    tftpHandle.fd = fs_open(fname, flags);

    syslog_printf("tftp %s opening file", tftpHandle.fd > 0 ? "success" : "failed");

    return(&tftpHandle);
}

void tftpClose(void* handle)
{
    struct tftp_handle *th = (struct tftp_handle *)handle;

    syslog_printf("tftp close: %d bytes transferred", th->totalLength);

    if (th->fd > 0) {
        fs_close(th->fd);
        th->fd = -1;
    }
}

int tftpRead(void* handle, void* buf, int bytes)
{
    struct tftp_handle *th = (struct tftp_handle *)handle;
    int len;

    if (th->fd > 0) {
        len = fs_read(th->fd, buf, bytes);
        if (len > 0) {
            tftpHandle.totalLength += len;
        }
    } else {
        len = 0;
    }

    return(len);
}

int tftpWrite(void* handle, struct pbuf* p)
{
    struct tftp_handle *th = (struct tftp_handle *)handle;
    struct pbuf *q;
    err_t err;
    int wlen;
    int len;

    len = 0;
    if (th->fd > 0) {
        for (q = p; q != NULL; q = q->next) {
            wlen = fs_write(th->fd, q->payload, q->len);
            if (wlen > 0) {
                len += wlen;
            }
        }
    }
    tftpHandle.totalLength += len;

    return(len);
}
```
