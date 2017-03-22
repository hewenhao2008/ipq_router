/**
**
**	Reliable Data Transfer over Serial Port (rdt)
**	Tx Routine
**
**/
#include "rdt_tx_thread.h"
#include <pthread.h>
#include "zb_serial_protocol.h"


#define TX_QUEUE_LEN	128
#define TX_QUEUE_MASK	0x7F

struct rdt_tx_queue
{
	serial_tx_buf_t * tx_queue[TX_QUEUE_LEN];
	
	pthread_mutex_t tx_queue_mutex;

	pthread_cond_t 	tx_avail_cond;		
	pthread_mutex_t tx_avail_mutex;	//when no data to send, sleep on this cond & mutex.

	zb_uint16 rd_idx;	//sending thread update this field.
	zb_uint16 wr_idx;	//caller update this field.
};

struct rdt_tx_retrans
{
	//tx
	zb_uint8 last_tx_seq;
	
	pthread_cond_t 	tx_acked;		//used for timeouted wait.
	pthread_mutex_t tx_mutex;	
	int	last_tx_acked;				//condition to check. tx set->0, rx set -> 1
	int tx_trans;					//retrans control.		

	//debug cnt.
	zb_uint32 	good_frame_cnt;
	zb_uint32   error_frame_cnt;	
};


struct serial_tx_mgt
{
	struct rdt_tx_queue  que;

	struct rdt_tx_retrans trans;	//transmit info control

	pthread_t thread_id;

	int flag;
	int status;	
};

struct serial_tx_mgt gTxMgt;


/**************** Serial Send with ACK check *****************************/

void print_current_time()
{
	struct  timeval tv;
	gettimeofday(&tv, NULL);

	ZB_PRINT(">>> time: sec=%u nsec=%u\n", tv.tv_sec, tv.tv_usec);
}

// 100ms
#define SERIAL_TIMEOUT_NS	1000*1000*100

//return 0 : send success.
// 		-1 : failed to send.  timeout 3 times.
static int rdt_data_robust_out(zb_uint8 *data, zb_uint8 len, zb_uint8 need_ack)
{
	struct timespec absTime;
	struct zb_ser_frame_hdr *phdr = (struct zb_ser_frame_hdr *)data;
	
	int rc = 0;
	int ret = 0;

#if 1
		//debug only.
		int i=0;
		ZB_DBG("--------Serial TX: ");
		for(i=0; i<len; i++)
		{
			ZB_DBG("%.2x ", data[i]&0xff);
		}
		ZB_DBG("-------------------------\n");
#endif

	gTxMgt.trans.tx_trans = 0;
	
SEND_AGAIN:
	pthread_mutex_lock( &gTxMgt.trans.tx_mutex );
	
	gTxMgt.trans.last_tx_acked = 0;
	
	gTxMgt.trans.last_tx_seq = phdr->seq;
	
	print_current_time();
	
	zb_send(data,len);

	if( !need_ack)
	{
		pthread_mutex_unlock( &gTxMgt.trans.tx_mutex );
		return 0;
	}

	clock_gettime(CLOCK_REALTIME, &absTime);  
	
	absTime.tv_nsec += SERIAL_TIMEOUT_NS;
	absTime.tv_sec += absTime.tv_nsec/1000000000;
	absTime.tv_nsec %= 1000000000;

	//rc = 0;		
	do{
		rc = pthread_cond_timedwait( &gTxMgt.trans.tx_acked, &gTxMgt.trans.tx_mutex, &absTime);	
				
	}while( (rc==0) && (!gTxMgt.trans.last_tx_acked) );	//wakeup unexpected.
	
	if( rc == 0)
	{
		//waked up before timeout. success.
		ret = 0;
	}
	else
	{
		//take all error as timeout.
		gTxMgt.trans.tx_trans++;
		
		if( gTxMgt.trans.tx_trans >= 3)
		{			
			ret = -1;	//Failed to send.
		}
		else
		{
			pthread_mutex_unlock( &gTxMgt.trans.tx_mutex );
			
			goto SEND_AGAIN;
		}		
	}

	pthread_mutex_unlock( &gTxMgt.trans.tx_mutex );

	return ret;
}

int is_ack_to_last_data(zb_uint8 seq)
{
	ZB_DBG("ACK seq=%x last_tx_seq=%x\n", seq,  gTxMgt.trans.last_tx_seq );
	return (seq == gTxMgt.trans.last_tx_seq) ? 1: 0;
}

//get ack from peer , wakup tx process.
void rdt_tx_data_acked(void)
{
	pthread_mutex_lock(&gTxMgt.trans.tx_mutex);

	gTxMgt.trans.last_tx_acked = 1;
	pthread_cond_signal( &gTxMgt.trans.tx_acked );
	
	pthread_mutex_unlock(&gTxMgt.trans.tx_mutex);	
}

////////////////////////////////////

//sleep when no data to send.
static void tx_thread_wait_data(void)
{
	int rc;

	pthread_mutex_lock(&gTxMgt.que.tx_avail_mutex);
	
	do{
		rc = pthread_cond_wait( &gTxMgt.que.tx_avail_cond, &gTxMgt.que.tx_avail_mutex);			
		
	}while( (rc==0) && (gTxMgt.que.rd_idx == gTxMgt.que.wr_idx) );	//wakeup unexpected.

	pthread_mutex_unlock(&gTxMgt.que.tx_avail_mutex);
}

//wakup tx thread when have to to send.
static void wakeup_tx_thread(void)
{
	pthread_mutex_lock( &gTxMgt.que.tx_avail_mutex );	
	pthread_cond_signal( &gTxMgt.que.tx_avail_cond );	
	pthread_mutex_unlock( &gTxMgt.que.tx_avail_mutex );	
}

int rdt_send(serial_tx_buf_t *pbuf)
{
	zb_uint16 wrn;	//next wr.
	zb_uint16 cur_wr;
	
	pthread_mutex_lock( &gTxMgt.que.tx_queue_mutex);

	cur_wr = gTxMgt.que.wr_idx;
	wrn = ( cur_wr + 1) & TX_QUEUE_MASK;
	
	if( wrn == gTxMgt.que.rd_idx)	//buffer full.
	{
		pthread_mutex_unlock( &gTxMgt.que.tx_queue_mutex);

		return -1;
	}
	else
	{
		gTxMgt.que.tx_queue[ cur_wr ] = pbuf;
		gTxMgt.que.wr_idx = wrn;
		
		pthread_mutex_unlock( &gTxMgt.que.tx_queue_mutex);
	}

	wakeup_tx_thread();

	return 0;	
}

void * serial_tx_thread(void *arg)
{
	serial_tx_buf_t *pbuf;
	
	ZB_DBG("---- serial_tx_thread start \n");

	while(1)
	{
		pthread_mutex_lock( &gTxMgt.que.tx_queue_mutex);

		if( gTxMgt.que.rd_idx == gTxMgt.que.wr_idx ) //queue empty.
		{
			pthread_mutex_unlock( &gTxMgt.que.tx_queue_mutex);		

			//sleep when no data output.
			tx_thread_wait_data();			
		}
		else
		{			
			pbuf = gTxMgt.que.tx_queue[ gTxMgt.que.rd_idx ];

			//printf(">>>>gTxMgt.que.wr_idx=%x gTxMgt.que.rd_idx=%x pbuf=%p\n", gTxMgt.que.wr_idx, gTxMgt.que.rd_idx, pbuf);

			//send buffer by serial port.
			rdt_data_robust_out( pbuf->dat, pbuf->dat_len, pbuf->need_ack );
			
			//update rd idx.
			gTxMgt.que.rd_idx = (gTxMgt.que.rd_idx + 1) & TX_QUEUE_MASK;

			pthread_mutex_unlock( &gTxMgt.que.tx_queue_mutex);	

			//free buffer cont.
			free(pbuf->dat);
			free(pbuf);
		}	
			
	}

	ZB_DBG("---- serial_tx_thread end \n");	
}

/******************************************************/

int init_rdt_tx(void)
{
	int ret;
	
	memset(&gTxMgt, 0, sizeof(struct serial_tx_mgt));
	
	pthread_mutex_init( &gTxMgt.que.tx_queue_mutex, NULL);
	
	pthread_cond_init(  &gTxMgt.que.tx_avail_cond, NULL );
	pthread_mutex_init( &gTxMgt.que.tx_avail_mutex, NULL);

	pthread_cond_init(  &gTxMgt.trans.tx_acked, NULL );
	pthread_mutex_init( &gTxMgt.trans.tx_mutex, NULL );

	ret = pthread_create( &gTxMgt.thread_id, NULL, serial_tx_thread, &gTxMgt);
	if( 0 != ret)
	{
		ZB_ERROR("Failed to create Thread : serial_tx_thread.\n");
		return -1;
	}

	return 0;
}


