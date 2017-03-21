#ifndef _WAN_TYPE_H_
#define _WAN_TYPE_H_

#include <linux/types.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/sockios.h>
#include <linux/version.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <time.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#include <error.h>


#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
#define MAC_STR_LEN 17

#define DEBUG_SWITCH 1
#if DEBUG_SWITCH
#define LOG_MSG printf
#define OUTPUT printf
#define DBG_PRINT printf
#else
#define LOG_MSG(...)
#define OUTPUT(...)
#define DBG_PRINT(...)
#endif

#define RET_VAR_IF(cond, var) do { \
	if (cond) \
	{ \
		return (var); \
	} \
}while(0)

#define MAX_RETRY_TIMES (10)
#define MAX_WAIT_SECONDS (10)
#define INPUT_PARA_NUM (5)

/*返回值定义*/
#define INTERNAL_ERROR (-2)
#define INPUT_PARA_ERROR (-1)
typedef enum
{
	WAN_NOT_LINK = 0,
	WAN_TYPE_DHCP,
	WAN_TYPE_STATIC_IP,
	WAN_TYPE_PPPOE,
	WAN_TYPE_802_1X_DHCP,
	WAN_TYPE_802_1X_STATIC_IP,
	WAN_TYPE_BPA,
	WAN_TYPE_L2TP,
	WAN_TYPE_PPTP,
	WAN_TYPE_DHCP_PLUS,
	WAN_TYPE_END
} WAN_TYPE;

#define DETECT_TYPE_BIT_DHCP  (0x1)
#define DETECT_TYPE_BIT_PPPOE (0x2)

typedef enum
{
	FALSE = 0,
	TRUE
}BOOL;

typedef int     STATUS;

typedef char	INT8;
typedef short	INT16;
typedef int		INT32;

typedef unsigned char	UINT8;
typedef unsigned short	UINT16;
typedef unsigned short	USHORT;
typedef unsigned long	UINT32;
typedef unsigned long	ULONG;


#endif /* _WAN_TYPE_H_ */

