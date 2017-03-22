#ifndef BIZ_MSG_PARSER_H
#define BIZ_MSG_PARSER_H

#include "common_def.h"
#include <sys/time.h>
#include <json-c/json.h>


/********************** Types used for internal only ********************/

/*****************************************************************/
typedef enum biz_msg_cmd_type
{
	CMD_WRITE,	// 写属性
	CMD_READ,	// 读属性
	CMD_ADD_DEV,  // 添加设备入网
	CMD_MAX_SUPPORTED,	
}biz_msg_cmd_type_t;


#define BIZ_MSG_MAX_MODEL_LEN	32
#define BIZ_MSG_MAX_UUID_LEN	64
#define BIZ_MSG_MAX_TOKEN_LEN	32
struct biz_msg_hdr
{
	biz_msg_cmd_type_t cmd_type;		//CMD_WRITE
	zb_uint8 	token[BIZ_MSG_MAX_TOKEN_LEN];
	
	//zb_uint8	model[BIZ_MSG_MAX_MODEL_LEN];
	//zb_uint8	dev_uuid[BIZ_MSG_MAX_UUID_LEN];
	//zb_uint16	short_id;	
};

#define BIZ_MSG_MAX_ATTR_NUM		16
#define BIZ_MSG_ATTR_MAXLEN			64
#define BIZ_MSG_ATTR_VAL_MAXLEN		64
struct biz_msg_attr
{
	zb_uint8 attr[BIZ_MSG_ATTR_MAXLEN];
	zb_uint8 val[BIZ_MSG_ATTR_VAL_MAXLEN];	
};

struct biz_msg_cmd_para
{
	zb_uint8 manufacture[BIZ_MSG_ATTR_MAXLEN];
	zb_uint8 module[BIZ_MSG_ATTR_MAXLEN];	
};


typedef struct biz_msg
{
	struct biz_msg_hdr hdr;

	zb_uint8 attr_cnt;
	
	union
	{
		struct biz_msg_attr  attrs[BIZ_MSG_MAX_ATTR_NUM];
		struct biz_msg_cmd_para	cmd_para;
	}u;
	
}biz_msg_t;


typedef int (*app_msg_handle_cb)(json_object *jobj,  biz_msg_t *pbiz_msg);

struct biz_cmd_string
{
	char *cmd;
	int len;
	biz_msg_cmd_type_t cmd_type;

	app_msg_handle_cb func;
};


int parse_app_msg(char *msg, biz_msg_t *pbiz_msg);

void dbg_show_biz_msg(biz_msg_t *pbiz_msg);


struct json_app_msg_resp * builder_app_msg_init();

int builder_app_msg_header_report(struct json_app_msg_resp *resp , char *cmd, char *uuid);

int builder_app_msg_data_append(struct json_app_msg_resp *resp, char *key, char *value);

int builder_app_msg_complete(struct json_app_msg_resp *resp);

int builder_test();


// internal used ///////////////////
struct json_app_msg_resp
{
	json_object *resp_obj;
	json_object *data_obj;	
};


#endif

