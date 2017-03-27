#include "msgque.h"

int msgsend(char *buf)
{
	int queue_id;
	IOT_MSG send_msg;
	
	queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
	if(queue_id == -1){
		fprintf(stderr, "iot_msg_queue_get error!\n");
		return 1;
	}

	send_msg.dst_mod = IOT_MODULE_EVENT_LOOP;
//	send_msg.dst_mod = IOT_MODULE_MQTT;
	send_msg.src_mod = IOT_MODULE_MQTT;
	send_msg.handler_mod = IOT_MODULE_NONE;
	strncpy(send_msg.body, buf, IOT_MSG_BODY_LEN_MAX);
	send_msg.body_len = strlen(send_msg.body)+1;
	if(iot_msg_send(queue_id, &send_msg)){
		fprintf(stderr, "iot_msg_send error!\n");
		return 1;
	}
	
	return 0;
}


int msgrecv(char *buf)
{
	int res;
	int queue_id;
	IOT_MSG recv_msg;
	
	queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
	if(queue_id == -1){
		fprintf(stderr, "iot_msg_queue_get error!\n"); 
		return 1;
	}

	recv_msg.dst_mod = IOT_MODULE_MQTT;

	res = iot_msg_recv_wait(queue_id, &recv_msg);
	if(res==-1){
		fprintf(stderr, "iot_msg_recv_wait error!\n");
		return 1;
	}
	strncpy(buf, recv_msg.body, IOT_MSG_BODY_LEN_MAX);

	return 0;
}

