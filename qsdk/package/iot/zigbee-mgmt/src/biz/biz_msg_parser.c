#include <json-c/json.h>
#include "biz_msg_parser.h"

#include "biz_msg_process.h"

static int handle_app_cmd_add_dev(json_object *json_param, biz_msg_t *pbiz_msg);

struct biz_cmd_string gBizCmds[] = {
	{.cmd="adddev", .len=5, .cmd_type=CMD_ADD_DEV, .func=handle_app_cmd_add_dev }	
};

#define IS_CMD(cmdstr, str)	(!strncmp(cmdstr, (str), strlen(str)))

int parser_app_msg_header_cmd(json_object *pj_msg, struct biz_msg_hdr *pbiz_msg_hdr)
{
	struct json_object *pj_obj;
	const char *cmd;
	int cmdlen;
	struct biz_cmd_string *pbiz_cmd;
	int i;
	int ret;
	
	ret = json_object_object_get_ex(pj_msg, "cmd", &pj_obj);
	if(ret != TRUE ) //json TRUE = 1
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	cmd = json_object_get_string(pj_obj);
	cmdlen = json_object_get_string_len(pj_obj);

	for(i=0; i<CMD_MAX_SUPPORTED; i++)
	{
		pbiz_cmd = &gBizCmds[i];
		
		if( (!strncmp(cmd, pbiz_cmd->cmd, pbiz_cmd->len)) && (cmdlen == pbiz_cmd->len))
		{
			pbiz_msg_hdr->cmd_type = pbiz_cmd->cmd_type;
			return 0;
		}
	}

	return -1;
}

#if 0
int parser_app_msg_header_model(json_object *pj_msg, struct biz_msg_hdr *pbiz_msg_hdr)
{
	struct json_object *pj_obj;
	const char *val;
	int val_len;
	json_bool ret;
	
	ret = json_object_object_get_ex(pj_msg, "model", &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	val = json_object_get_string(pj_obj);
	val_len = json_object_get_string_len(pj_obj);	

	strncpy( pbiz_msg_hdr->model, val,  BIZ_MSG_MAX_MODEL_LEN );
	pbiz_msg_hdr->model[BIZ_MSG_MAX_MODEL_LEN-1] = '\0';

	return 0;
}
#endif

#if 0
int parser_app_msg_header_uuid(json_object *pj_msg, struct biz_msg_hdr *pbiz_msg_hdr)
{
	struct json_object *pj_obj;
	const char *val;
	int val_len;
	json_bool ret;
	
	ret = json_object_object_get_ex(pj_msg, "device_uuid", &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	val = json_object_get_string(pj_obj);
	val_len = json_object_get_string_len(pj_obj);	

	strncpy( pbiz_msg_hdr->dev_uuid, val,  BIZ_MSG_MAX_UUID_LEN );
	pbiz_msg_hdr->dev_uuid[BIZ_MSG_MAX_UUID_LEN-1] = '\0';

	return 0;
}
#endif

int parser_app_msg_header_token(json_object *pj_msg, struct biz_msg_hdr *pbiz_msg_hdr)
{
	struct json_object *pj_obj;
	const char *val;
	int val_len;
	json_bool ret;
	
	ret = json_object_object_get_ex(pj_msg, "token", &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	val = json_object_get_string(pj_obj);
	val_len = json_object_get_string_len(pj_obj);	

	strncpy( pbiz_msg_hdr->token, val,  BIZ_MSG_MAX_TOKEN_LEN);
	pbiz_msg_hdr->token[BIZ_MSG_MAX_TOKEN_LEN-1] = '\0';

	return 0;
}

#if 0
int parser_app_msg_header_short_id(json_object *pj_msg, struct biz_msg_hdr *pbiz_msg_hdr)
{
	struct json_object *pj_obj;	
	json_bool ret;
	
	ret = json_object_object_get_ex(pj_msg, "short_id", &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_int))
	{
		return -1;
	}

	pbiz_msg_hdr->short_id = json_object_get_int(pj_obj);
	
	return 0;
}
#endif

// data content: {"attr1":"val1"}
// 			 or: {"attr1":"val1", "attr2":"val2"}
int parser_app_msg_data_wrcmd(json_object *pj_msg, biz_msg_t *pbiz_msg)
{
	struct json_object *pj_obj;	
	struct json_object *pj_dat_obj;	
	const char *data_str;

	struct json_object_iterator it;
	struct json_object_iterator itEnd;
	struct json_object* obj;
	
	const char *attr;
	const char *attr_val;
	struct json_object *attr_val_obj;
	json_bool ret;
	int ret_val = 0;
	
	ret = json_object_object_get_ex(pj_msg, "data", &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	data_str = json_object_get_string(pj_obj);
	
	pj_dat_obj= json_tokener_parse(data_str);
	if( pj_dat_obj == NULL)
	{
		return -1;
	}

	//extrace all key/pairs in the data object.
	it = json_object_iter_begin(pj_dat_obj);
	itEnd = json_object_iter_end(pj_dat_obj);

	while( !json_object_iter_equal(&it, &itEnd) )
	{
		attr = json_object_iter_peek_name(&it);
		attr_val_obj = json_object_iter_peek_value(&it);

		if( !json_object_is_type(attr_val_obj, json_type_string))
		{
			ret_val = -1;
			break; //attr val error.
		}
		attr_val = json_object_get_string(attr_val_obj);

		strncpy( pbiz_msg->u.attrs[pbiz_msg->attr_cnt].attr, attr, BIZ_MSG_ATTR_MAXLEN);
		pbiz_msg->u.attrs[pbiz_msg->attr_cnt].attr[BIZ_MSG_ATTR_MAXLEN-1] = '\0';

		strncpy( pbiz_msg->u.attrs[pbiz_msg->attr_cnt].val, attr_val, BIZ_MSG_ATTR_MAXLEN);
		pbiz_msg->u.attrs[pbiz_msg->attr_cnt].val[BIZ_MSG_ATTR_MAXLEN-1] = '\0';

		pbiz_msg->attr_cnt++;

		ZB_DBG(" attribute:%s  value:%s\n", attr, attr_val);
		
		json_object_iter_next(&it);
	}

	json_object_put(pj_dat_obj);	
	json_object_put(pj_msg);

	return ret_val;
}

#define MAX_CMD_LEN	16
int parser_app_msg_data_get_cmd(json_object *pj_msg, char *cmd)
{
	struct json_object *pj_obj;		
	const char *data_str;

	json_bool ret;
	int ret_val = 0;
	int val_len;
	
	ret = json_object_object_get_ex(pj_msg, "cmd", &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	data_str = json_object_get_string(pj_obj);		
	val_len = json_object_get_string_len(pj_obj);	
	
	if( val_len >= MAX_CMD_LEN )
		val_len = MAX_CMD_LEN ;
		
	strncpy(cmd, data_str, val_len );
	cmd[MAX_CMD_LEN - 1] = '\0';
	
	return ret_val;
}

int parser_app_msg_header(json_object *param_obj, struct biz_msg_hdr *pbiz_msg_hdr)
{
	int ret;
	
	ret = parser_app_msg_header_token(param_obj, pbiz_msg_hdr );
	if( ret != 0)
		return ret;

	return 0;	
}

int handle_app_cmd_write(json_object *pj_msg, biz_msg_t *pbiz_msg)
{
	parser_app_msg_data_wrcmd(pj_msg, pbiz_msg);	
}

//return length of string.
int parser_get_string(json_object *obj, char *key, char *str, int len)
{
	struct json_object *pj_obj;
	const char *val;
	int val_len;
	json_bool ret;
	
	ret = json_object_object_get_ex(obj, key, &pj_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	if( !json_object_is_type(pj_obj, json_type_string))
	{
		return -1;
	}

	val = json_object_get_string(pj_obj);
	val_len = json_object_get_string_len(pj_obj);	

	if(val_len >= len  )
		val_len = len ;
		
	strncpy( str, val,	val_len);
	str[len -1] = '\0';
	
	return val_len;
}

static int handle_app_cmd_add_dev(json_object *json_param, biz_msg_t *pbiz_msg)
{
	json_object *pj_dat_obj;
	json_object *pj_dat_cont_obj;
	const char *data_str;
	json_bool ret;
	int len;
	
	ret = json_object_object_get_ex(json_param, "data", &pj_dat_obj);
	if(ret != TRUE)
	{		
		return -1;
	}
	
	if( !json_object_is_type(pj_dat_obj, json_type_string))
	{
		return -1;
	}

	data_str = json_object_get_string(pj_dat_obj);
	
	pj_dat_cont_obj= json_tokener_parse(data_str);
	if( pj_dat_cont_obj == NULL)
	{
		return -1;
	}

	/************ analyze the content of data ************************/
	len = parser_get_string(pj_dat_cont_obj, "manufacture", pbiz_msg->u.cmd_para.manufacture, BIZ_MSG_ATTR_MAXLEN);
	if( len < 0)
	{
		json_object_put(pj_dat_cont_obj);
		ZB_ERROR(">> Failed to get para [manufacture]\n");
		return -1;
	}
	
	parser_get_string(pj_dat_cont_obj, "module", pbiz_msg->u.cmd_para.module, BIZ_MSG_ATTR_MAXLEN);
	if( len < 0)
	{
		json_object_put(pj_dat_cont_obj);
		ZB_ERROR(">> Failed to get para [module]\n");
		return -1;
	}

	json_object_put(pj_dat_cont_obj);

	//call biz function here.
	biz_search_dev( pbiz_msg->hdr.token, "NNNN", 
					pbiz_msg->u.cmd_para.manufacture , 
					pbiz_msg->u.cmd_para.module );

	return 0;
}


int parse_app_msg(char *msg, biz_msg_t *pbiz_msg)
{
	json_object *pj_msg;
	json_object *param_obj;
	int ret = 0;
	char cmd[MAX_CMD_LEN];
	int i=0;
	
	ZB_DBG("msg:%s\n", msg);
	
	pj_msg = json_tokener_parse(msg);
	if(pj_msg == NULL)
	{
		ZB_DBG("json_fail_at  %s : %d\n", __FILE__, __LINE__);
		return -1;
	}

	//get params.
	ret = json_object_object_get_ex(pj_msg, "params", &param_obj);
	if(ret != TRUE)
	{		
		return -1;
	}

	ret = parser_app_msg_header(param_obj, &pbiz_msg->hdr);
	if( ret != 0)
	{	
		json_object_put(pj_msg);
		
		ZB_DBG("json_fail_at  %s : %d\n", __FILE__, __LINE__);
		return ret;	
	}

	parser_app_msg_data_get_cmd(param_obj, cmd);

	//find which command we get.
	for(i=0; i<sizeof(gBizCmds)/sizeof(struct biz_cmd_string); i++)
	{
		ZB_DBG(">> APP_CMD is :%s\n", cmd);
		
		if( IS_CMD(cmd, gBizCmds[i].cmd) )
		{
			//dispatch app cmd.
			gBizCmds[i].func( param_obj,  pbiz_msg );
		}
	}


	json_object_put(pj_msg);
	
	return ret;
}

//=======================================================



//---------------------------------------------------
void dbg_show_biz_msg(biz_msg_t *pbiz_msg)
{
	int i=0;
	
	ZB_PRINT("-------------show biz message structure info--------------\n");
	ZB_PRINT("cmd_type: %d\n", pbiz_msg->hdr.cmd_type);
	//ZB_PRINT("model:    (%s)\n", pbiz_msg->hdr.model);
	//ZB_PRINT("dev_uuid: (%s)\n", pbiz_msg->hdr.dev_uuid);
	ZB_PRINT("token:    (%s)\n", pbiz_msg->hdr.token);
	//ZB_PRINT("short_id: %u\n", pbiz_msg->hdr.short_id);

	ZB_PRINT("---- attr_cnt: %u----\n", pbiz_msg->attr_cnt);
	for(i=0; (i<pbiz_msg->attr_cnt)&&(i<BIZ_MSG_MAX_ATTR_NUM); i++)
	{
		ZB_PRINT("attr: (%s)  ", pbiz_msg->u.attrs[i].attr);
		ZB_PRINT("val:  (%s)  \n", pbiz_msg->u.attrs[i].val);
	}
	printf("\n");	
}


//=========================================


int builder_app_msg_header_report(struct json_app_msg_resp *resp , char *cmd, char *uuid)
{	
    time_t timestamp;

	time(&timestamp);   	 
	
	json_object_object_add(resp->resp_obj, "cmd", json_object_new_string(cmd));
	json_object_object_add(resp->resp_obj, "uuid", json_object_new_string(uuid));
	json_object_object_add(resp->resp_obj, "timestamp", json_object_new_int(timestamp));
	
	return 0;	
}

int builder_app_msg_data_append(struct json_app_msg_resp *resp, char *key, char *value)
{
	json_object_object_add( resp->data_obj, key, json_object_new_string(value) );		
	return 0;
}

struct json_app_msg_resp * builder_app_msg_init()
{
	struct json_app_msg_resp *resp;

	resp = malloc( sizeof(struct json_app_msg_resp));
	if( resp == NULL)
		return NULL;

	resp->resp_obj = json_object_new_object();
	if( !resp->resp_obj )
	{
		free(resp);
		return NULL;
	}
	
	resp->data_obj = json_object_new_object();
	if( !resp->data_obj )
	{
		json_object_put(resp->resp_obj );
		
		free(resp);
		return NULL;
	}

	return resp;
}


int builder_app_msg_complete(struct json_app_msg_resp *resp)
{
	json_object_object_add(resp->resp_obj, "data", resp->data_obj);

	ZB_DBG(">JOSN_RESULT:%s\n", json_object_to_json_string(resp->resp_obj));
	
	json_object_put(resp->data_obj);
	json_object_put(resp->resp_obj);

	free(resp);	
}

int builder_test()
{
	struct json_app_msg_resp * resp = builder_app_msg_init() ;

	if(!resp)
	{
		ZB_ERROR(">> Failed to init app msg.\n");
		return -1;
	}

	builder_app_msg_header_report(resp, "report", "001122334455667701");
	
	builder_app_msg_data_append(resp, "CurrentX", "6000");

	builder_app_msg_complete(resp);

	return 0;
}

