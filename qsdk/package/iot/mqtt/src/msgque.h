#ifndef _IOT_EVENT_DEMO_H_
#define _IOT_EVENT_DEMO_H_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iot_msg_util.h>

int msgsend(char *buf);
int msgrecv(char *buf);

#endif
