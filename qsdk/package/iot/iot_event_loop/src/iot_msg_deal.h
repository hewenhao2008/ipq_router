#ifndef _IOT_MSG_DEAL_H_
#define _IOT_MSG_DEAL_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <iot_msg_util.h>

#define IOT_CTRL_PARAM_LEN_MAX  255

int msg_deal(IOT_MSG *rcv_msg, IOT_MSG *snd_msg);

typedef struct iot_ctrl_cmd_info
{
	char method[IOT_CTRL_PARAM_LEN_MAX];
	char uuid[IOT_CTRL_PARAM_LEN_MAX];
}IOT_CTRL_CMD_INFO;


typedef struct iot_ctrl_method_tr_rule
{
	char *method;
	unsigned char dst_mod;
}IOT_CTRL_METHOD_TR_RULE;


#endif