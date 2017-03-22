#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <pthread.h>

struct cli_msg_queue_info
{
	int queue_id;

	pthread_t  thread_id;	
	
	//biz_msg_in_cb func;
	
	//IOT_MSG cli_msg;		
};


#define CLI_MSG_CONT_LEN	2048

typedef struct cli_msg
{
	long mtype;
	char data[2048];
}cli_msg_t;


#define QUEUE_TYPE_TOSER	100
#define QUEUE_TYPE_TOCLI	101
#define CMD_ZIGBEE	5

typedef int (* func_cb)(int argc, char *argv[]);

struct inter_command
{
	int cmd_id;
	int sub_cmd_id;
	
	char *name;
	func_cb func;
	int argv; 	//argument counter.
	char *desc;

	char data[2000];
};


int cli_msg_queue_get(int queue_no);
int cli_msg_send(int queue_id, cli_msg_t *msg, int msgContLen);
int cli_msg_rcv(int queue_id, cli_msg_t *msg, long MsgType);

#endif

