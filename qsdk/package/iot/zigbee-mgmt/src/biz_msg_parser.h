#ifndef BIZ_MSG_PARSER_H
#define BIZ_MSG_PARSER_H

#include "common_def.h"

/********************** Types used for internal only ********************/
/*
{
  "cmd": "write",
  "model": "qualcomm.plug.v1",
  "device_uuid": "158d0000e7ccfd",
  "token": "5",
  "short_id": 42653,
  "data": "{\"status\":\"toggle\"}"
 }
*/

typedef enum biz_msg_cmd_type
{
	CMD_WRITE,	// 写属性

	CMD_READ,	// 读属性 

	CMD_ADD_DEV,  // 添加设备入网

	CMD_MAX_SUPPORTED,
	
}biz_msg_cmd_type_t;

struct biz_cmd_string
{
	char *cmd;
	int len;
	biz_msg_cmd_type_t cmd_type;
};



/*****************************************************************/
#define BIZ_MSG_MAX_MODEL_LEN	32
#define BIZ_MSG_MAX_UUID_LEN	64
#define BIZ_MSG_MAX_TOKEN_LEN	32
struct biz_msg_hdr
{
	biz_msg_cmd_type_t cmd_type;	//CMD_WRITE
	zb_uint8	model[BIZ_MSG_MAX_MODEL_LEN];
	zb_uint8	dev_uuid[BIZ_MSG_MAX_UUID_LEN];
	zb_uint8 	token[BIZ_MSG_MAX_TOKEN_LEN];
	zb_uint16	short_id;	
};

#define BIZ_MSG_MAX_ATTR_NUM		16
#define BIZ_MSG_ATTR_MAXLEN			64
#define BIZ_MSG_ATTR_VAL_MAXLEN		64
struct biz_msg_attr
{
	zb_uint8 attr[BIZ_MSG_ATTR_MAXLEN];
	zb_uint8 val[BIZ_MSG_ATTR_VAL_MAXLEN];	
};

typedef struct biz_msg
{
	struct biz_msg_hdr hdr;

	zb_uint8 attr_cnt;
	
	union
	{
		struct biz_msg_attr  attrs[BIZ_MSG_MAX_ATTR_NUM];
		
	}u;
	
}biz_msg_t;


int parse_app_msg(char *msg, biz_msg_t *pbiz_msg);

void dbg_show_biz_msg(biz_msg_t *pbiz_msg);

#endif

