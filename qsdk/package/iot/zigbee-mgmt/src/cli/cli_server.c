#include "cli_common.h"
#include "common_def.h"

#include "zb_serial_protocol.h"
#include "zb_cmds.h"
#include "zb_cmds_light.h"
#include "cli_common.h"
#include <stdio.h>
#include "cli_server.h"

struct cli_msg_queue_info g_cli_msg_que;

static int zigbee_help(int argc, char *argv[]);


int zigbee_join(int argc, char *argv[])
{
	printf("zigbee_join argc=%d.\n", argc);

	biz_search_dev("tttoken", "1234","LDS", "ZHA-ColorLight");	
}

int zigbee_turn_on_off(int argc, char *argv[])
{
	int on=1;
	zb_uint16 short_addr;
	zb_uint8 endp_addr;
	struct serial_session_node *pNode;
	ser_addr_t endp;

	ZB_DBG("argc=%d\n", argc);
	
	if( !strncmp(argv[1], "on", 2) )
	{
		on = 1;
	}
	else if( !strncmp(argv[1], "off", 3) )
	{
		on = 0;
	}
	else if( !strncmp(argv[1], "toggle", strlen("toggle")) )
	{
		on = 2;
	}

	short_addr = strtoul(argv[2], NULL, 0);
	endp_addr =  strtoul(argv[3], NULL, 0);

	endp.short_addr = short_addr;
	endp.endp_addr = endp_addr;
	
	pNode = find_serial_session( host_get_hostaddr(), &endp);
	if( pNode == NULL)
	{
		printf("> Not found this endpoint. short_addr:%x endp:%x\n", short_addr, endp_addr);
		return -1;
	}
	
	zb_onoff_light_new(pNode, on);

	return 0;
}

int zigbee_move_to_color(int argc, char *argv[])
{
	int on=1;
	zb_uint16 short_addr;
	zb_uint8 endp_addr;
	struct serial_session_node *pNode;
	ser_addr_t endp;
	zb_uint16 colorX, colorY, transitionTime;

	ZB_DBG("argc=%d\n", argc);
	
	
	short_addr = strtoul(argv[1], NULL, 0);
	endp_addr =  strtoul(argv[2], NULL, 0);
	
	colorX =  strtoul(argv[3], NULL, 0);
	colorY =  strtoul(argv[4], NULL, 0);
	transitionTime =  strtoul(argv[5], NULL, 0);

	ZB_DBG("CLI Info: short_addr:%x endp:%x colorX:%x colorY:%x transitionTime:%x\n",
			short_addr, endp_addr, colorX, colorY, transitionTime);

	endp.short_addr = short_addr;
	endp.endp_addr = endp_addr;
	
	pNode = find_serial_session( host_get_hostaddr(), &endp);
	if( pNode == NULL)
	{
		printf("> Not found this endpoint. short_addr:%x endp:%x\n", short_addr, endp_addr);
		return -1;
	}

	zb_light_move_to_color(pNode, colorX, colorY, transitionTime);	

	return 0;
}


int zigbee_move_to_color_temperature(int argc, char *argv[])
{
	int on=1;
	zb_uint16 short_addr;
	zb_uint8 endp_addr;
	struct serial_session_node *pNode;
	ser_addr_t endp;
	zb_uint16 temperature, transitionTime;

	ZB_DBG("argc=%d\n", argc);
	
	
	short_addr = strtoul(argv[1], NULL, 0);
	endp_addr =  strtoul(argv[2], NULL, 0);
	
	temperature =  strtoul(argv[3], NULL, 0);	
	transitionTime =  strtoul(argv[4], NULL, 0);

	ZB_DBG("CLI Info: short_addr:%x endp:%x temperature:%x transitionTime:%x\n",
			short_addr, endp_addr, temperature, transitionTime);

	endp.short_addr = short_addr;
	endp.endp_addr = endp_addr;
	
	pNode = find_serial_session( host_get_hostaddr(), &endp);
	if( pNode == NULL)
	{
		printf("> Not found this endpoint. short_addr:%x endp:%x\n", short_addr, endp_addr);
		return -1;
	}

	zb_light_move_to_color_temperature(pNode, temperature, transitionTime);	

	return 0;
}


int zigbee_move_to_level(int argc, char *argv[])
{
	int on=1;
	zb_uint16 short_addr;
	zb_uint8 endp_addr;
	struct serial_session_node *pNode;
	ser_addr_t endp;
	zb_uint16 transitionTime;
	zb_uint8 level;

	ZB_DBG("argc=%d\n", argc);
		
	short_addr = strtoul(argv[1], NULL, 0);
	endp_addr =  strtoul(argv[2], NULL, 0);
	
	level =  strtoul(argv[3], NULL, 0);	
	transitionTime =  strtoul(argv[4], NULL, 0);

	ZB_DBG("CLI Info: short_addr:%x endp:%x level:%x transitionTime:%x\n",
			short_addr, endp_addr, level, transitionTime);

	endp.short_addr = short_addr;
	endp.endp_addr = endp_addr;
	
	pNode = find_serial_session( host_get_hostaddr(), &endp);
	if( pNode == NULL)
	{
		printf("> Not found this endpoint. short_addr:%x endp:%x\n", short_addr, endp_addr);
		return -1;
	}

	zb_light_move_to_level(pNode, level, transitionTime);	

	return 0;
}


/********************************************/

static char * args[256];
static int inited = 0;
int init_args()
{
	int i=0;
	
	if(!inited)
	{	
		for(i=0; i<256; i++)
		{
			args[i] = malloc(256);
			if(!args[i])
			{
				return -1;
			}
			memset(args[i], 0, 256);
		}
	}
	else
	{
		for(i=0; i<256; i++)
		{			
			memset(args[i], 0, 256);
		}
	}

	return 0;
}

int parse_cmd(char *line)
{
	int s=0;
	int cur_arg=0, pos=0;
	int prev_is_white = 0;
	int cnt=0;
	char *pstr;

	while( line[s] != '\0' )
	{
		if( line[s] == ' ' || line[s] == '\t' )
		{
			if( prev_is_white )
			{
				//skip continues white.
				s++;
				continue;
			}
			else
			{
				cur_arg++;
				pos=0;
				cnt++;
				prev_is_white = 1;
			}
		}
		else
		{
			pstr = args[cur_arg];
			pstr[pos] = line[s];
			pos++;
			prev_is_white = 0;
		}
		
		s++;
	}

	pstr = args[cur_arg];
	if(  (pstr[0] == ' ') || (pstr[0] == '\t')  )
	{
		return cnt;
	}
	else
	{
		return cnt + 1;
	}
}

///////////////////////////////////////////////////////////////////

struct inter_command zigbee_cmds[] = {
	{CMD_ZIGBEE, 0, "help", zigbee_help, 0, "show zigbee helps\n"},
	{CMD_ZIGBEE, 1, "join manufactoryId moduleId", zigbee_join, 0, "Permit join start\n"},
	{CMD_ZIGBEE, 2, "turn on/off shortId endpId", zigbee_turn_on_off, 3, "Turn on/off one device\n"},	
	{CMD_ZIGBEE, 3, "move_to_color shortaddr endpaddr colorX colorY transitionTIme", zigbee_move_to_color, 6, "change color of light\n"},
	{CMD_ZIGBEE, 3, "move_to_color_temp shortaddr endpaddr temp transitionTIme", zigbee_move_to_color_temperature, 5, "change color temperature\n"},
	{CMD_ZIGBEE, 3, "move_to_level shortaddr endpaddr level transitionTIme", zigbee_move_to_level, 5, "change level (darkness or brightness)\n"},
};

char buf[2048];
static int zigbee_help(int argc, char *argv[])
{
	int cnt;
	int offset = 0;
	
	for(cnt=0; cnt < sizeof(zigbee_cmds)/sizeof(struct inter_command); cnt++)
	{
		offset = snprintf(buf + offset, 2048, "%s : %s", zigbee_cmds[cnt].name,zigbee_cmds[cnt].desc);
	}
	buf[offset+1] = '\0';
	
	cli_msg_send_to_cli(buf);

	return 0;
}


int exec_internal_cmd(char *arg)
{
	int argc;
	int cnt;

	ZB_DBG("> arg:%s\n", arg);
	
	argc = init_args();

	argc = parse_cmd(arg);
	if( argc == 0)
	{
		ZB_DBG("No argument.\n");
		return 0;
	}	

	for(cnt=0; cnt < sizeof(zigbee_cmds)/sizeof(struct inter_command); cnt++)
	{
		if( !memcmp(zigbee_cmds[cnt].name, args[0], strlen(args[0]) ) )
		{
			printf("calling %s : argc=%d", zigbee_cmds[cnt].name, argc);
			
			//call function
			zigbee_cmds[cnt].func( argc, args );
			break;
		}
	}
	
	return 0;	
}
//////////////////////////////////////////////////////////////////////////

int cli_msg_handle(char *data, int dataLen)
{
	struct inter_command *pcmd = (struct inter_command *)data;

	ZB_DBG("> cli_msg_handle Get msg from cli. dataLen=%d\n", dataLen);
	ZB_DBG("> pcmd->cmd_id=%d \n", pcmd->cmd_id);

	if( pcmd->cmd_id == CMD_ZIGBEE)
	{
		exec_internal_cmd( pcmd->data );
	}

	return 0;	
}

void * cli_msg_loop(void *arg)
{	
	cli_msg_t msg;
	int len;

	ZB_DBG("------cli_msg_loop start----\n");
	
	while(1)
	{
		len = cli_msg_rcv( g_cli_msg_que.queue_id, &msg, QUEUE_TYPE_TOSER);
		if( len > 0)
		{
			cli_msg_handle(msg.data, len);
		}
	}
}

void cli_msg_send_to_cli(char *str)
{
	cli_msg_t msg;
	int len;
	
	msg.mtype = QUEUE_TYPE_TOCLI;
	len = snprintf( str, 2048, "%s", str);

	cli_msg_send(g_cli_msg_que.queue_id, &msg, len);
}

int init_cli_msg_handle(void)
{
	int ret;
	
	memset(&g_cli_msg_que, 0, sizeof(struct cli_msg_queue_info));
	
	g_cli_msg_que.queue_id = cli_msg_queue_get(0);
	if( -1 == g_cli_msg_que.queue_id)
	{
		ZB_ERROR("iot_msg_queue_get(0) failed, queueid:%d err:%s\n", 
					g_cli_msg_que.queue_id, strerror(errno));
		return -1;
	}
	ZB_DBG("---cli queu_id:%d\n", g_cli_msg_que.queue_id);

	ret = pthread_create( &g_cli_msg_que.thread_id, NULL, cli_msg_loop, NULL);
	if( 0 != ret)
	{
		ZB_ERROR("Failed to create Thread : cli_msg_rcv.");
		return -1;
	}

	return 0;	
}



