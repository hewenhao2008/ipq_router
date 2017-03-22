/**
**	Reliable Data Transfer over Serial (rdt)
**	Rx routine.
**
**/
#include "common_def.h"
#include <pthread.h>

#include "serial_dev.h"
#include "zb_serial_protocol.h"

struct rdt_input
{
	zb_uint8	ori_buffer[SBUF_LEN];	

	pthread_t serial_input_thread;
	int serial_thread_kill_flag ;
	int serial_thread_status ; //default S_THREAD_NOT_START;

	//statistics info.
	zb_uint32	bytes;
};
struct rdt_input gSerInput;


static void * rdt_recv_thread(void *arg)
{
	struct rdt_input *pInput;
	int nr;
	int i=0;
	
	pInput = (struct rdt_input *)arg;

	ZB_PRINT("---- start_serial_input_thread running...\n");

	gSerInput.serial_thread_status = S_THREAD_RUNNING;
	
	while(1)
	{
		nr = zb_serial_read_blocking( pInput->ori_buffer, SBUF_LEN);
		if( gSerInput.serial_thread_kill_flag )
		{
			break;
		}

		if( nr > 0)
		{	
			//process buffer data.
			serial_data_in( pInput->ori_buffer, nr);					
		}	
	}

	gSerInput.serial_thread_status = S_THREAD_STOP;
	ZB_PRINT("---- start_serial_input_thread stop...\n");
	
}

int start_rdt_recv_thread(void)
{	
	int ret;	
	
	ret = pthread_create(&gSerInput.serial_input_thread, NULL, rdt_recv_thread, &gSerInput );
	if( 0 != ret)
	{
		ZB_ERROR("Failed to create serial rx thread.");
		return -1;
	}	
	
	return 0;
}

//Join to thread, wait thread exit.
void join_rdt_recv_thread(void)
{
	pthread_join( gSerInput.serial_input_thread, NULL);
}


void stop_rdt_recv_thread(void)
{
	int i=0;
	
	gSerInput.serial_thread_kill_flag = 1;

	for(i=0; i<10; i++)
	{
		if( S_THREAD_RUNNING == gSerInput.serial_thread_status)
		{
			sleep(2);	//serial read timeout in 10 sec. 
		}
	}
}

