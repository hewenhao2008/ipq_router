#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <sys/ipc.h>

#include "iot_typedef.h"
#include "iot_msg_util.h"
#include "iot_debug_util.h"


static ULONG g_debug_level = IOT_DEBUG_LEVEL_OFF;
static ULONG g_debug_mod = 0;
static INT g_debug_qid = -1;


ULONG iot_debug_init(ULONG module)
{
    int qid = iot_msg_queue_get(IOT_MSG_SQUEUE_DEBUG);
    if (qid < 0)
    {
        printf("%s %x fail.\n", __func__, module);
        return ERROR;
    }

    g_debug_mod = module;
    g_debug_qid = qid;

    return OK;
}


ULONG iot_debug_exit(void)
{
    return OK;
}


VOID iot_debug_level_set(ULONG level)
{
    g_debug_level = level;
}


ULONG iot_debug_send(ULONG way, const CHAR* buf)
{
#if 0
    IOT_DEBUG_MSG_S msg;
    msg.dst = IOT_MODULE_DEBUG;
    msg.src = g_debug_mod;
    msg.way = way;
    strncpy(msg.buf, buf, IOT_DEBUG_MAX_LEN);
    msg.buf[IOT_DEBUG_MAX_LEN - 1] = '\0';

    if(-1 == msgsnd(g_debug_qid, (void *)&msg, sizeof(msg) - sizeof(long), 0))
	{
		printf("%s return %s\n", __func__, strerror(errno));
		return ERROR;
	}
#endif

    /* debug进程开发完成之前，先直接在本地printf */
    printf("%s", buf);
    return OK;
}


ULONG iot_debug_print(ULONG level, ULONG way, const CHAR* fmt, ...)
{
    char buf[IOT_DEBUG_MAX_LEN];

    if (level <= g_debug_level)
        return OK;

    va_list ap;
    va_start(ap, fmt);
	vsnprintf(buf, IOT_DEBUG_MAX_LEN - 1,  fmt, ap);
    buf[IOT_DEBUG_MAX_LEN - 1] = '\0';
	va_end(ap);

    return iot_debug_send(way, buf);
}



