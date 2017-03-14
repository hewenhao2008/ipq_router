#include "iot_msg_util.h"


int iot_msg_queue_get(IOT_MSG_SQUEUE_NO queue_no)
{
	key_t key = ftok("/usr/lib/libiot_util.so", queue_no);
	if(key == -1)
	{
		fprintf(stderr, "libiot_util: ftok get msg key failed, err=%s\n", strerror(errno));
		return -1;		
	}

	int qid = msgget(key, 0);
	if(qid == -1)
	{
		qid = msgget(key, IPC_CREAT|0666);
	}

	return qid;	
}

int iot_msg_send(int queue_id, IOT_MSG * msg)
{
	int ret = msgsnd(queue_id, msg, IOT_MSG_REAL_LEN(msg), 0);
	return ret;
}

int iot_msg_recv_wait(int queue_id, IOT_MSG * msg)
{
	int ret = msgrcv(queue_id, msg, IOT_MSG_MAX_LEN, msg->dst_mod,0);
	return ret;
}

int iot_msg_recv_nowait(int queue_id, IOT_MSG * msg)
{
	int ret = msgrcv(queue_id, msg, IOT_MSG_MAX_LEN, msg->dst_mod,IPC_NOWAIT);
	return ret;
}