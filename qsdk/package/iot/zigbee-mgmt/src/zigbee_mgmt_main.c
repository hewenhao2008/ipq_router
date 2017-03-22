#include "common_def.h"
#include <pthread.h>

#include "serial_dev.h"
#include "zb_serial_protocol.h"
#include "rdt_rx_thread.h"
#include "cli_server.h"

#include "zb_db.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char *argv[])
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


	//pthread_join(serial_input_thread, NULL);
	join_rdt_recv_thread();

	ZB_PRINT(">> ERROR: main thread exit..<<\n");
	return 0;	
}
