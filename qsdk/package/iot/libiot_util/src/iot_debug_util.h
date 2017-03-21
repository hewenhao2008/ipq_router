
#ifndef __IOT_DEBUG_UTIL_H__
#define __IOT_DEBUG_UTIL_H__

typedef enum
{
    IOT_DEBUG_LEVEL_OFF     = 0x0,
    IOT_DEBUG_LEVEL_ERROR   = 0x1,
    IOT_DEBUG_LEVEL_WARNING = 0x2,
    IOT_DEBUG_LEVEL_INFO    = 0x3,
}IOT_DEBUG_LEVEL_E;


typedef enum
{
    IOT_DEBUG_WAY_COM = 0x1,
    IOT_DEBUG_WAY_WEB = 0x2,
    IOT_DEBUG_WAY_ALL = (IOT_DEBUG_WAY_COM | IOT_DEBUG_WAY_WEB)
}IOT_DEBUG_WAY_E;


#define IOT_DEBUG_MAX_LEN   1024

typedef struct
{
    ULONG dst;
    ULONG src;
    ULONG way;
    ULONG buf[IOT_DEBUG_MAX_LEN];
}IOT_DEBUG_MSG_S;


#define IOT_DEBUG(level, way, fmt, args...)\
    iot_debug_print(level, way, "[%s %d]"fmt, __func__, __LINE__, ##args)


ULONG iot_debug_init(ULONG module);
ULONG iot_debug_exit(void);
VOID iot_debug_level_set(ULONG level);
ULONG iot_debug_print(ULONG level, ULONG way, const CHAR* fmt, ...);

#endif

