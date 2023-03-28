// XMODEM for eLua
#include <stdbool.h>

#ifndef __XMODEM_H__
#define __XMODEM_H__

// xmodem data receive callback
typedef int (*p_xm_data_func)(unsigned char *data, unsigned size, bool final, void *usr);

// xmodem data i/o
typedef void ( *p_xm_send_func )( unsigned char, void * );
typedef int ( *p_xm_recv_func )( int, void * );

// xmodem timeout/retry parameters
#define XMODEM_TIMEOUT                5000000
#define XMODEM_RETRY_LIMIT            60

// error return codes
#define XMODEM_ERROR_NONE             (0)
#define XMODEM_ERROR_REMOTECANCEL     (-1)
#define XMODEM_ERROR_OUTOFSYNC        (-2)
#define XMODEM_ERROR_RETRYEXCEED      (-3)
#define XMODEM_ERROR_OUTOFMEM         (-4)
#define XMODEM_ERROR_CALLBACK         (-5)
#define XMODEM_ERROR_GENERIC          (-6)

long xmodem_receive(p_xm_data_func callback, void *usr,
    p_xm_send_func send, p_xm_recv_func recv);

#endif // #ifndef __XMODEM_H__
