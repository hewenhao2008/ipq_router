#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "common_def.h"
#include "biz_msg_in.h"
#include "cli_common.h"

char *stripwhite (char *string);

struct cli_msg_queue_info gCliMsgQ;

int init_cli_msg_queue()
{
	memset(&gCliMsgQ, 0, sizeof(struct cli_msg_queue_info));
	
	gCliMsgQ.queue_id = cli_msg_queue_get(0);
	if( -1 == gCliMsgQ.queue_id)
	{
		ZB_ERROR("iot_msg_queue_get(0) failed, queueid:%d err:%s\n", 
					gCliMsgQ.queue_id, strerror(errno));
		return -1;
	}

	ZB_DBG("CLI queue_id:%d\n", gCliMsgQ.queue_id );

	return 0;	
}

int main(int argc, char *argv[])
{
	int done=0;
	char *line, *s;

	init_cli_msg_queue();
	
	initialize_readline();	//bind our completer

	for(; done==0;)
	{
		line = readline("> ");

		if(!line)
			break;

		s = stripwhite(line);

		if( *s)
		{
			add_history(s);
			execute_line(s);
		}

		free(line);
	}

	return 0;
}

