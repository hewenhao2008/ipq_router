#include "common_def.h"
#include <pthread.h>

#include "serial.h"
#include "zb_serial_protocol.h"


struct ser_input
{
	zb_uint8	ori_buffer[SBUF_LEN];
	//int 		ori_buf_len;
	
	zb_uint8	data[SBUF_LEN];
	int 		data_len;
	
};
struct ser_input gSerInput;


pthread_t serial_input_thread;
int serial_thread_kill_flag = 0;

void * recv_serial(void *arg)
{
	struct ser_input *pSerInput;
	int nr;
	int i=0;
	
	pSerInput = (struct ser_input *)arg;

	ZB_PRINT("---- start_serial_input_thread running...\n");
	
	while(1)
	{
		nr = zb_serial_read_blocking( pSerInput->ori_buffer, SBUF_LEN);
		if( serial_thread_kill_flag )
		{
			break;
		}
		
		if( nr > 0)
		{	
			ZB_DBG("nr=%d\n", nr);
			for(i=0; i<nr; i++)
			{
				ZB_DBG("%x ", pSerInput->ori_buffer[i]&0xFF);
			}		
			
			//process buffer data.
			serial_data_in( pSerInput->ori_buffer, nr);					
		}		
	}

	ZB_PRINT("---- start_serial_input_thread stop...\n");
	
}

int start_serial_input_thread()
{
	serial_input_thread = pthread_create(&serial_input_thread, NULL, recv_serial, &gSerInput );
	if( 0 != serial_input_thread)
	{
		ZB_ERROR("Failed to create serial rx thread.");
		return -1;
	}

	return 0;
}

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

	ret = init_zb_serial_protocol_mgmt();
	if( ret != 0)
	{
		return -1;
	}
	
	ret = start_serial_input_thread();
	if( ret != 0)
	{
		return -1;
	}

	printf(">> start test() \n");
	test();

	pthread_join(serial_input_thread, NULL);

	ZB_PRINT(">> ERROR: main thread exit..\n");
	return 0;	
}
