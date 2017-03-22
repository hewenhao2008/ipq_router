#include "cli_common.h"
#include "common_def.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>

int cli_msg_queue_get(int queue_no)
{
	key_t key = ftok("/usr/bin/zigbee-mgmt", queue_no);
	if( key == -1)
	{
		ZB_ERROR("ftok get msg key failed, err=%s\n", strerror(errno));
		return -1;
	}

	int qid = msgget(key, 0);
	if( qid == -1)
	{
		qid = msgget(key, IPC_CREAT|0666);
	}
	
	return qid;	
}

int cli_msg_send(int queue_id, cli_msg_t *msg, int msgContLen)
{
	int ret;
	ret = msgsnd(queue_id , msg, msgContLen , 0 );
	if( ret != 0)
	{
		ZB_ERROR("Failed to send cli msg, err=%s\n", strerror(errno));
	}
	return ret;
}

int cli_msg_rcv(int queue_id, cli_msg_t *msg, long MsgType)
{
	int cnt;

	//msgtype == 0, get the first msg in the queue.
	cnt = msgrcv(queue_id , msg, 2048, MsgType, 0 );
	if( cnt <= 0)
	{
		ZB_ERROR("Failed to rcv cli msg, err=%s\n", strerror(errno));
	}
	return cnt;
}



