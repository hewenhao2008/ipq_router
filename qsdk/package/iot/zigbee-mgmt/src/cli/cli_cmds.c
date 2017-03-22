#include "common_def.h"
#include <stdio.h>
#include <string.h>
#include "cli_common.h"

extern struct cli_msg_queue_info gCliMsgQ;

int com_zigbee(char *arg)
{
	cli_msg_t msg;
	cli_msg_t msg_rcv;
	
	struct inter_command *pcmd;
	
	int msgContLen = 0;	
	
	printf("com_zigbee:%s\n",arg);

	if(!arg)
	{
		return 0;
	}

	memset( &msg, 0, sizeof(cli_msg_t) );
	
	msg.mtype = QUEUE_TYPE_TOSER;
	pcmd = (struct inter_command *)msg.data;
	pcmd->cmd_id = CMD_ZIGBEE;
	
	memcpy( pcmd->data, arg, strlen(arg) );
	
	cli_msg_send(gCliMsgQ.queue_id, &msg , 2048) ;

	//wait for response.
	cli_msg_rcv(gCliMsgQ.queue_id, &msg_rcv, QUEUE_TYPE_TOCLI);
	printf("%s", msg_rcv.data );

	return 0;
}

