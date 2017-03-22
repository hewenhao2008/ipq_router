#include <unistd.h>
#include <stdio.h>
#include <pthread.h>

#include "common_def.h"
#include "serial_dev.h"
#include "zb_serial_protocol.h"
#include "rdt_rx_thread.h"

#include "biz_msg_parser.h"


void test1(void)
{	
	char cmd[5] = {0x11,0x22,0x33,0x44,0x55};
	char payload[3] = {0x38,0x38,0x38};
	int payload_len = 3;

	ser_addr_t app;
	ser_addr_t zb_end;
	struct serial_session_node *pSession;
	int ret;

	printf("-----------case1: send one frame by a session -----------------\n");

	app.short_addr = 0x00;
	app.endp_addr = 0xaa;
	
	zb_end.short_addr = 0x11;
	zb_end.endp_addr = 0xbb;
	

	printf("--- add serial session.\n");
	pSession = find_or_add_serial_session(&app, &zb_end);
	if( pSession == NULL)
	{
		printf(">>> Failed to create session.\n");
		return;
	}

	printf(">>>> calling zb_protocol_send.\n");
	ret = zb_protocol_send(pSession, cmd, payload, payload_len);
	if( ret  == 0)
	{
		printf(">> send success.\n");
	}
	else
	{
		printf(">> send fail.\n");
	}

	printf("------case 1 test end -----\n");
		
}



int test1_main()
{	
	int ret;
	
	//init serial.
	ret = zb_init_serial();
	if( ret != 0)
	{
		return -1;
	}

	
	ret = init_rdt_tx();
		if( ret != 0)
		{
			return -1;
		}

	ret = init_zb_serial_protocol_mgmt();
	if( ret != 0)
	{
		return -1;
	}

	ret = init_dev_search_thread();
	if( ret != 0)
	{
		return -1;
	}
	
	ret = start_rdt_recv_thread();
	if( ret != 0)
	{
		return -1;
	}

	
	//start dev search thread.
	ret = start_dev_search_thread();
	if( ret != 0)
	{
		return -1;
	}

	printf("-----test1-----\n");
	test1();


	//pthread_join(serial_input_thread, NULL);
	join_rdt_recv_thread();

	ZB_PRINT(">> ERROR: main thread exit..<<\n");
	return 0;	
}

////////////////////////////////////////////////////////////////////////////////
int test3_main()
{	
	int ret;
	
	//init serial.
	ret = zb_init_serial();
	if( ret != 0)
	{
		return -1;
	}

	ret = init_rdt_tx();
	if( ret != 0)
	{
		return -1;
	}

	ret = init_zb_serial_protocol_mgmt();
	if( ret != 0)
	{
		return -1;
	}

	ret = init_dev_search_thread();
	if( ret != 0)
	{
		return -1;
	}

	ret = init_zigbee_db();
	if( ret != 0)
	{
		ZB_ERROR("Failed to init. init_zigbee_db.\n");
		return -1;
	}

	//biz layer, init node mgmt.
	ret = biz_node_mgmt_init();
	if( ret != 0)
	{
		ZB_ERROR("Failed to init. biz_node_mgmt_init.\n");
		return -1;
	}

	//init biz msg queue.
	ret = init_biz_msg_queue();
	if( ret != 0)
	{
		ZB_ERROR("Failed to init. init_biz_msg_queue.\n");
		return -1;
	}
		
	//start dev search thread.
	ret = start_dev_search_thread();
	if( ret != 0)
	{
		return -1;
	}

	//Start to recv data now..
	ret = start_rdt_recv_thread();
	if( ret != 0)
	{
		return -1;
	}

	ret = init_cli_msg_handle();
	if( ret != 0)
	{
		return -1;
	}

	ZB_DBG("-------------initialize complete.--------------\n");

	test3();

	//pthread_join(serial_input_thread, NULL);
	join_rdt_recv_thread();

	ZB_PRINT(">> ERROR: main thread exit..<<\n");
	return 0;	
}


/////////////////////////////////////////////////////////////////////////////////

char * js_wr_attr = "{\
    \"cmd\": \"write\",\
    \"model\": \"qualcomm.plug.v1\",\
    \"device_uuid\": \"158d0000e7ccfd\",\
    \"token\": \"5\",\
    \"short_id\": 42653,\
    \"data\": \"{\\\"status\\\":\\\"toggle\\\"}\"\
    }";

char * js_wr_attr2 = "{\
    \"cmd\": \"write\",\
    \"model\": \"qualcomm.plug.v1\",\
    \"device_uuid\": \"158d0000e7ccfd\",\
    \"token\": \"5\",\
    \"short_id\": 42653,\
    \"data\": \"{\\\"status\\\":\\\"toggle\\\",\\\"rgb\\\":\\\"123\\\"}\"\
    }";

/*
char * add_dev_attr2 = "{\
    \"cmd\": \"write\",\
    \"model\": \"qualcomm.plug.v1\",\
    \"device_uuid\": \"158d0000e7ccfd\",\
    \"token\": \"5\",\
    \"short_id\": 42653,\
    \"data\": \"{\\\"status\\\":\\\"toggle\\\",\\\"rgb\\\":\\\"123\\\"}\"\
    }";
*/
/*  
char * add_dev_attr2 = "{\
  \"client\": \"android\", \
  \"timestamp\": 1487236771,\
  \"version\": 1.1,\
  \"client_version\": \"android5.0\",\
  \"user_agent\": \"huawei\",\
  \"method\": \"control\",\
  \"token\": \"adfljasdklfjawefmdfamsdfklajdf\",\
  \"params\": {\
    \"cmd\": \"adddev\",   \
    \"token\": \"5555\",    \
    \"data\": \"{\\\"manufacture\\\":\\\"LDS\\\", \\\"module\\\":\\\"ZHA-ColorLight\\\"}\"\
    }\
 }";    
*/


char *add_dev_attr2 =   "{\
    \"client\": \"android\",\
    \"timestamp\": 1487236771,\
    \"version\": 1.1,\
    \"client_version\":\"android5.0\",\
    \"user_agent\": \"huawei\",\
    \"method\": \"control\",\
    \"token\": \"adfljas\",\
     \"params\": {\
     \"cmd\": \"adddev\", \
     \"token\": \"5555\",    \
     \"data\": \"{\\\"manufacture\\\":\\\"LDS\\\", \\\"module\\\":\\\"ZHA-ColorLight\\\"}\"\
     }\
 } ";
      

void test2(void)
{
	int ret;
	biz_msg_t bizMsg;

	memset(&bizMsg, 0, sizeof(bizMsg));
	
	ret = parse_app_msg(add_dev_attr2, &bizMsg);
	if( ret < 0)
	{
		printf(" !!! Failed to parse !!!\n");
		return ;
	}

	dbg_show_biz_msg(&bizMsg);
	
	printf(">>>>>>> success.<<<<<<<<<\n\n");
	return;
}

void test3(void)
{
	//biz_search_dev(0x1234, "FIB");
	int ret;
	biz_msg_t bizMsg;

	memset(&bizMsg, 0, sizeof(bizMsg));
	
	ret = parse_app_msg(add_dev_attr2, &bizMsg);
	if( ret < 0)
	{
		printf(" !!! Failed to parse !!!\n");
		return ;
	}
	
}


/////////////////////////////////////////////////////////////////////////////////

void usage(void)
{
	printf("usage:\n");
	printf("\t -t 1 : case1 : send a packet with retransmit enabled.\n");
	printf("\t -t 2 : case2 : json parser test.\n");
	printf("\t -t 3 : case3 : search/add device test.\n");
}


int main(int argc, char *argv[])
{
	int ret;
	int ch;
	int case_num;
	
	while( (ch=getopt(argc, argv, "t:h")) != -1 )
	{
		printf("optind: %d\n", optind);
		printf("option: %c , optarg:%s\n", ch , optarg);
		
		switch(ch)
		{
			case 't':
				case_num = strtoul(optarg, NULL, 0);
				printf("---- choosed case [case_num]----\n", case_num);
				break;
				
			case 'h':
				usage();
				return 0;
			default:
				usage();
				return 0;
		}
	}
	
	switch(case_num)
	{
		case 1: 
			test1_main();
			break;
		case 2:
			test2();
			break;
		case 3:
			test3_main();
			break;
		case 4:
			builder_test();
			break;
		default:
			break;
	}

	
	return 0;	
}


