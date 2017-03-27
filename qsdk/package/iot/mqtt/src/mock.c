#include "client_shared.h"
#include "msgque.h"
#include "parse.h"

int send(char *buf)
{
	int queue_id;
	IOT_MSG send_msg;
	
	queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
	if(queue_id == -1){
		fprintf(stderr, "iot_msg_queue_get error!\n");
		return 1;
	}

	send_msg.dst_mod = IOT_MODULE_MQTT;
	send_msg.src_mod = IOT_MODULE_EVENT_LOOP;
	send_msg.handler_mod = IOT_MODULE_NONE;
	strncpy(send_msg.body, buf, IOT_MSG_BODY_LEN_MAX);
	send_msg.body_len = strlen(send_msg.body)+1;
	if(iot_msg_send(queue_id, &send_msg)){
		fprintf(stderr, "iot_msg_send error!\n");
		return 1;
	}
	
	return 0;
}


int recv(char *buf)
{
	int res;
	int queue_id;
	IOT_MSG recv_msg;
	
	queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
	if(queue_id == -1){
		fprintf(stderr, "iot_msg_queue_get error!\n"); 
		return 1;
	}

	//recv_msg.dst_mod = IOT_MODULE_MQTT;
	recv_msg.dst_mod = IOT_MODULE_EVENT_LOOP;

	res = iot_msg_recv_wait(queue_id, &recv_msg);
	if(res==-1){
		fprintf(stderr, "iot_msg_recv_wait error!\n");
		return 1;
	}
	strncpy(buf, recv_msg.body, IOT_MSG_BODY_LEN_MAX);

	return 0;
}

int main(int argc, char *argv[])
{
	int rc;
	char buf[IOT_MSG_BODY_LEN_MAX];
	char buf1[IOT_MSG_BODY_LEN_MAX];
	int method;

	struct mosq_config cfg;

	//从队列中获取消息,然后通过mqtt发送 
	while(1){  
		printf("\nwaitting ...\n");
		rc = recv(buf);
		if(rc){
			fprintf(stderr, "msgrecv error!\n");
		}
		printf(">> [%s] and parse\n", buf);

		method = parse(buf);
		printf("method= [%d]\n", method);

		rc = client_config_load(&cfg, CLIENT_SUB, argc, argv);

		printf("cfg.test_result= [%d]\n", cfg.test_result);

		rc = package(method, cfg.test_result, buf1);
		if(rc){
			fprintf(stderr, "package error!\n");
			continue;
		}
		printf("buf1= [%s] and sending ...\n", buf1);
		rc = send(buf1);
		if(rc){
			fprintf(stderr, "msgsend error!\n");
		}
	}

}
