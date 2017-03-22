#include "biz_msg_in.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "common_def.h"

struct biz_msg_queue_info
{
	int queue_id;

	biz_msg_in_cb func;
	pthread_t  thread_id;
	
	IOT_MSG zigbee_msg;	
	IOT_MSG zigbee_msg_tx;	
};

struct biz_msg_queue_info gBizMsgQueInfo;

static void clear_msg_queue(int queue_id, unsigned char dst_mod)
{	
	int res;	
	IOT_MSG msg;

	while(1){		
		msg.dst_mod = dst_mod;		
		res = iot_msg_recv_nowait(queue_id, &msg);		
		if(res<0)			
			break;	
	}
}

static int dummy_biz_msg_handle(unsigned char *msg, unsigned int msg_len)
{
	ZB_DBG("R: msg_len=%u\n", msg_len);
	return 0;
}

int register_biz_msg_in_cb(biz_msg_in_cb cb)
{
	gBizMsgQueInfo.func = cb;
}

int un_register_biz_msg_in_cb(biz_msg_in_cb cb)
{
	gBizMsgQueInfo.func = dummy_biz_msg_handle;
}

void * biz_msg_queue_rx_thread(void *arg)
{
	struct biz_msg_queue_info *pBizMsgQInfo;
	int ret;
	
	pBizMsgQInfo = (struct biz_msg_queue_info *)arg;

	ZB_DBG("biz_msg_queue_rx_thread start.\n");
	
	while(1)
	{
		pBizMsgQInfo->zigbee_msg.dst_mod = IOT_MODULE_ZIGBEE;
		
		ret = iot_msg_recv_wait(pBizMsgQInfo->queue_id, &pBizMsgQInfo->zigbee_msg);
		if( ret > 0)
		{
			pBizMsgQInfo->func( pBizMsgQInfo->zigbee_msg.body, pBizMsgQInfo->zigbee_msg.body_len);	
		}	
	}	

	ZB_DBG("biz_msg_queue_rx_thread exit.\n");
}

//return 0 on success
// -1 on fail.
int biz_msg_send(unsigned char *data, int len)
{
	int ret;
	
	gBizMsgQueInfo.zigbee_msg_tx.dst_mod = IOT_MODULE_MQTT;	
	gBizMsgQueInfo.zigbee_msg_tx.src_mod = IOT_MODULE_ZIGBEE;
	gBizMsgQueInfo.zigbee_msg_tx.handler_mod = IOT_MODULE_NONE;	
	
	memcpy(gBizMsgQueInfo.zigbee_msg_tx.body, data, len);		
	gBizMsgQueInfo.zigbee_msg_tx.body_len = len;

	ret = iot_msg_send(gBizMsgQueInfo.queue_id, &gBizMsgQueInfo.zigbee_msg_tx);
	return ret;
}

//return 0 on success
// -1 on fail.
int biz_msg_send_nocpoy(biz_buff_t *buff)
{
	int ret;

	buff->msg.dst_mod = IOT_MODULE_MQTT;	
	buff->msg.src_mod = IOT_MODULE_ZIGBEE;
	buff->msg.handler_mod = IOT_MODULE_NONE;	
	
	ret = iot_msg_send(gBizMsgQueInfo.queue_id, &buff->msg);	
	return ret;
}

int init_biz_msg_queue(void)
{
	int ret;
	
	memset(&gBizMsgQueInfo, 0, sizeof(struct biz_msg_queue_info) );
	
	register_biz_msg_in_cb(dummy_biz_msg_handle);
	
	gBizMsgQueInfo.queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
	if( -1 == gBizMsgQueInfo.queue_id)
	{
		ZB_ERROR("iot_msg_queue_get(IOT_MODULE_EVENT_LOOP) failed, queueid:%d err:%s\n", 
					gBizMsgQueInfo.queue_id, strerror(errno));
		return -1;
	}

	//clear all msg in queue.
	clear_msg_queue(gBizMsgQueInfo.queue_id, IOT_MODULE_ZIGBEE);
	
	ret = pthread_create( &gBizMsgQueInfo.thread_id, NULL, biz_msg_queue_rx_thread, &gBizMsgQueInfo);
	if( 0 != ret)
	{
		ZB_ERROR("Failed to create Thread : biz_msg_queue_rx.");
		return -1;
	}
	
	return 0;	
}


