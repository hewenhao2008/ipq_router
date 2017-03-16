#include <json-c/json.h>
#include "biz_msg_parser.h"

struct biz_cmd_string gBizCmds[CMD_MAX_SUPPORTED] = {
	{.cmd="write", .len=5, .cmd_type=CMD_WRITE },
	
};


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
	json_object_put(pj_dat_obj);

	return ret_val;
}



int parser_app_msg_header(json_object *pj_msg, struct biz_msg_hdr *pbiz_msg_hdr)
{
	int ret;
	ret = parser_app_msg_header_cmd(pj_msg, pbiz_msg_hdr );
	if( ret != 0)
	{
		ZB_DBG("json_fail_at  %s : %d\n", __FILE__, __LINE__);
		return ret;
	}
	
	ret = parser_app_msg_header_model(pj_msg, pbiz_msg_hdr );
	if( ret != 0)
		return ret;

	ret = parser_app_msg_header_uuid(pj_msg, pbiz_msg_hdr );
	if( ret != 0)
		return ret;

	ret = parser_app_msg_header_token(pj_msg, pbiz_msg_hdr );
	if( ret != 0)
		return ret;

	ret = parser_app_msg_header_short_id(pj_msg, pbiz_msg_hdr );
	if( ret != 0)
		return ret;

	return 0;
	
}

int parse_app_msg(char *msg, biz_msg_t *pbiz_msg)
{
	json_object *pj_msg;	
	int ret = 0;

	ZB_DBG("msg:%s\n", msg);
	
	pj_msg = json_tokener_parse(msg);
	if(pj_msg == NULL)
	{
		ZB_DBG("json_fail_at  %s : %d\n", __FILE__, __LINE__);
		return -1;
	}

	ret = parser_app_msg_header(pj_msg, &pbiz_msg->hdr);
	if( ret != 0)
	{	
		json_object_put(pj_msg);
		
		ZB_DBG("json_fail_at  %s : %d\n", __FILE__, __LINE__);
		return ret;	
	}

	//write command data.
	ret = parser_app_msg_data_wrcmd(pj_msg, pbiz_msg);
	if( ret != 0)
	{
		ZB_DBG("json_fail_at  %s : %d\n", __FILE__, __LINE__);
	}
	

	json_object_put(pj_msg);
	
	return ret;
}


//---------------------------------------------------
void dbg_show_biz_msg(biz_msg_t *pbiz_msg)
{
	int i=0;
	
	ZB_PRINT("-------------show biz message structure info--------------\n");
	ZB_PRINT("cmd_type: %d\n", pbiz_msg->hdr.cmd_type);
	ZB_PRINT("model:    (%s)\n", pbiz_msg->hdr.model);
	ZB_PRINT("dev_uuid: (%s)\n", pbiz_msg->hdr.dev_uuid);
	ZB_PRINT("token:    (%s)\n", pbiz_msg->hdr.token);
	ZB_PRINT("short_id: %u\n", pbiz_msg->hdr.short_id);

	ZB_PRINT("---- attr_cnt: %u----\n", pbiz_msg->attr_cnt);
	for(i=0; (i<pbiz_msg->attr_cnt)&&(i<BIZ_MSG_MAX_ATTR_NUM); i++)
	{
		ZB_PRINT("attr: (%s)  ", pbiz_msg->u.attrs[i].attr);
		ZB_PRINT("val:  (%s)  \n", pbiz_msg->u.attrs[i].val);
	}
	printf("\n");	
}


