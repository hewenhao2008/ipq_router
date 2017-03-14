#include "iot_msg_deal.h"

IOT_CTRL_METHOD_TR_RULE  g_iot_ctrl_method_tr_tbl[] = 
{
//	{"getWanConfig", 			IOT_MODULE_DATABASE, },  
//	{"getWlanDeviceList", 		IOT_MODULE_DATABASE, },  
//	{"isValidRouter", 			IOT_MODULE_DATABASE, },  
//	{"isRouterSetupDone", 		IOT_MODULE_DATABASE, },  
//	{"setADSLConfig", 			IOT_MODULE_DATABASE, },  
//	{"setSSIDConfig", 			IOT_MODULE_DATABASE, },  
//	{"setSSIDConfig_5G", 		IOT_MODULE_DATABASE, },  
	{"getRoomDevices", 			IOT_MODULE_DATABASE, },  
	{"getWLANDevices", 			IOT_MODULE_DATABASE, },  
	{"getDeviceNetworkSpeed",	IOT_MODULE_NONE, },  
	{"setDeviceAccess", 		IOT_MODULE_NONE, },  
	{"getDeviceProperty", 		IOT_MODULE_NONE, },  
	{"control", 				IOT_MODULE_NONE, },  
	{NULL, 						IOT_MODULE_NONE, },  
};


static unsigned char analy_dst_module(IOT_MSG *rcv_msg);
static int get_iot_ctrl_info(IOT_MSG *msg, char *method_buf, int method_buf_len,
								char *uuid_buf, int uuid_buf_len);
static unsigned char query_dst_module_database(char *method, char *uuid);

int msg_deal(IOT_MSG *rcv_msg, IOT_MSG *snd_msg)
{
	*snd_msg = *rcv_msg;
/*	
	if(!IOT_MODULE_IS_NONE(rcv_msg->dst_mod))
	{
		snd_msg->dst_mod = rcv_msg->dst_mod;
	}
*/		
	unsigned char dst_mod = analy_dst_module(rcv_msg);
	if(dst_mod == IOT_MODULE_NONE)
	{
		return -1;
	}
	snd_msg->dst_mod = dst_mod;

	return 0;
}

static unsigned char analy_dst_module(IOT_MSG *rcv_msg)
{
	int i, res;
	char method_buf[IOT_CTRL_PARAM_LEN_MAX];
	char uuid_buf[IOT_CTRL_PARAM_LEN_MAX];
	unsigned char dst = IOT_MODULE_NONE;

	switch(rcv_msg->src_mod)
	{
		case IOT_MODULE_DATABASE:
		case IOT_MODULE_WIFI:
		case IOT_MODULE_ZIGBEE:
			dst = IOT_MODULE_MQTT;
			return dst;
		case IOT_MODULE_EVENT_LOOP:
		case IOT_MODULE_NONE:
		default:
			return IOT_MODULE_NONE;

		case IOT_MODULE_MQTT:
			break;
	}

	// rcv_msg->src_mod == IOT_MODULE_MQTT
	res = get_iot_ctrl_info(rcv_msg, method_buf, IOT_CTRL_PARAM_LEN_MAX,
								uuid_buf, IOT_CTRL_PARAM_LEN_MAX);
	if(res<0){
		fprintf(stderr, "get iot ctrl info from json string failed\n");
		return IOT_MODULE_NONE; 
	}
	
	i=0;
	while(1)
	{
		if(g_iot_ctrl_method_tr_tbl[i].method == NULL)
		{
			return IOT_MODULE_NONE; 		
		}
	
		if(strcmp(g_iot_ctrl_method_tr_tbl[i].method, method_buf)==0){
			dst = g_iot_ctrl_method_tr_tbl[i].dst_mod;
			break;
		}
		i++;
	}
	if(dst == IOT_MODULE_NONE)
	{
		dst = query_dst_module_database(method_buf, uuid_buf);
	}

	return dst;
}

static int get_iot_ctrl_info(IOT_MSG *msg, char *method_buf, int method_buf_len,
								char *uuid_buf, int uuid_buf_len)
{
	msg->body[IOT_MSG_BODY_LEN_MAX-1] = 0;
	return get_method_uuid_from_json(msg->body, method_buf, method_buf_len, uuid_buf, uuid_buf_len);
}


static unsigned char query_dst_module_database(char *method, char *uuid)
{
	unsigned char dst = IOT_MODULE_NONE;
	fprintf(stderr, "query dst module from database(method=%s; uuid=%s)\n", method, uuid);

	dst = (uuid[0]%2)==0?IOT_MODULE_ZIGBEE:IOT_MODULE_WIFI;

	return dst;
}