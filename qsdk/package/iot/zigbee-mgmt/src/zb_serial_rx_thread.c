#include "common_def.h"
#include <pthread.h>

#include "serial.h"
#include "zb_serial_protocol.h"

enum SERIAL_THREAD_STATUS
{
	S_THREAD_NOT_START,
	S_THREAD_RUNNING,
	S_THREAD_STOP
};

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
int serial_thread_status = S_THREAD_NOT_START;

void * recv_serial(void *arg)
{
	struct ser_input *pSerInput;
	int nr;
	int i=0;
	
	pSerInput = (struct ser_input *)arg;

	ZB_PRINT("---- start_serial_input_thread running...\n");

	serial_thread_status = S_THREAD_RUNNING;
	
	while(1)
	{
		nr = zb_serial_read_blocking( pSerInput->ori_buffer, SBUF_LEN);
		if( serial_thread_kill_flag )
		{
			break;
		}

		if( nr > 0)
		{				
			#if 0
				ZB_DBG("nr=%d\n", nr);
				for(i=0; i<nr; i++)
				{
					ZB_DBG("%x ", pSerInput->ori_buffer[i]&0xFF);
				}		
			#endif
			
			//process buffer data.
			serial_data_in( pSerInput->ori_buffer, nr);					
		}		
		
	}

	serial_thread_status = S_THREAD_STOP;
	ZB_PRINT("---- start_serial_input_thread stop...\n");
	
}

int start_serial_input_thread(void)
{	
	int ret;
	ZB_DBG(">> start_serial_input_thread in.\n");
	
	ret = pthread_create(&serial_input_thread, NULL, recv_serial, &gSerInput );
	if( 0 != ret)
	{
		ZB_ERROR("Failed to create serial rx thread.");
		return -1;
	}
	ZB_DBG(">> start_serial_input_thread out.\n");
	
	return 0;
}

void join_serial_rx_thread(void)
{
	pthread_join(serial_input_thread, NULL);
}

void stop_serial_input_thread(void)
{
	int i=0;
	
	serial_thread_kill_flag = 1;

	for(i=0; i<10; i++)
	{
		if( S_THREAD_RUNNING == serial_thread_status)
		{
			sleep(2);	//serial read timeout in 10 sec. 
		}
	}
}

