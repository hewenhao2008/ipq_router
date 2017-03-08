#include "common_def.h"
#include "zb_serial_protocol.h"
#include <pthread.h>

typedef enum e_ZBP_STATE
{
	S_IDLE,		//idle, waitting for start of frame.
	S_SOP_BEGIN,	//get 0x55
	
	S_HDR,		//recving data, till all S_BODY received.
	S_DATA,
	S_CRC_BEGIN,
	S_CRC_END,
	
}ZBP_STATE;

struct zb_protocol_mgmt
{
	ZBP_STATE state;
	
	zb_uint8	data[SBUF_LEN];
	int 		data_len;
	zb_uint8	crc[2];

	//tx
	struct serial_session_node *pCurTxNode; //which session node is sending frame.
	pthread_cond_t 	tx_acked;		//used for timeouted wait.
	pthread_mutex_t tx_mutex;	
	int	last_tx_acked;				//condition to check. tx set->0, rx set -> 1
	int tx_trans;					//retrans control.		

	//debug cnt.
	zb_uint32 	good_frame_cnt;
	zb_uint32   error_frame_cnt;	
};

struct zb_protocol_mgmt	gZbProtMgmt;

struct serial_session_mgmt gSerialSessionMgmt;

int init_zb_serial_protocol_mgmt()
{
	int i=0;

	ZB_DBG("> init_zb_serial_protocol_mgmt.\n");
	
	memset(&gZbProtMgmt, 0, sizeof(struct zb_protocol_mgmt));
	gZbProtMgmt.state = S_IDLE;

	pthread_cond_init( &gZbProtMgmt.tx_acked, NULL );
	pthread_mutex_init( &gZbProtMgmt.tx_mutex, NULL);

	memset(&gSerialSessionMgmt, 0, sizeof(struct serial_session_mgmt));
	for(i=0; i<SER_SESSION_HASH_NUM; i++)
	{
		INIT_LIST_HEAD( &gSerialSessionMgmt.list[i] );
	}

	return 0;
}

/*
  16bit CRC notes:
  "CRC-CCITT"
    poly is g(X) = X^16 + X^12 + X^5 + 1  (0x1021)
    used in the FPGA (green boards and 15.4)
    initial remainder should be 0xFFFF
//[[
// This CRC seems to take about the same amount of time as the table driven CRC
// which was timed at 34 cycles on the mega128 (8.5us @4MHz) and it utilizes
// much less flash.  
//]]
*/
zb_uint16 gen_crc(zb_uint8 *data, zb_uint8 len)
{
	int i=0;
	zb_uint16 prevResult = 0xFFFF;
	
	for(i=0; i<len; i++)
	{
		prevResult = ((zb_uint16) (prevResult >> 8)) | ((zb_uint16) (prevResult << 8));
		prevResult ^= data[i];
		prevResult ^= (prevResult & 0xff) >> 4;
		prevResult ^= (zb_uint16) (((zb_uint16) (prevResult << 8)) << 4);

		//[[ What I wanted is the following function:
		// prevResult ^= ((prevResult & 0xff) << 4) << 1;
		// Unfortunately the compiler does this in 46 cycles.	The next line of code
		// does the same thing, but the compiler uses only 10 cycles to implement it.
		//]]
		prevResult ^= ((zb_uint8) (((zb_uint8) (prevResult & 0xff)) << 5)) |
			((zb_uint16) ((zb_uint16) ((zb_uint8) (((zb_uint8) (prevResult & 0xff)) >> 3)) << 8));
	 }
	
	 return prevResult;
}

//
int serial_data_in(zb_uint8 *data, zb_uint8 len)
{
	int i=0;
	zb_uint16  crc;
	struct zb_ser_frame_hdr *phdr;

	for(i=0; i<len; i++)
	{		
		switch( gZbProtMgmt.state )
		{
			case S_IDLE:
				if( data[i] == 0xAA)
				{
					gZbProtMgmt.state = S_SOP_BEGIN;
				}				
				break;
			case S_SOP_BEGIN:
				if( data[i] == 0x55 )
				{
					gZbProtMgmt.state = S_HDR;
					gZbProtMgmt.data[0] = 0xAA;
					gZbProtMgmt.data[1] = 0x55;
					gZbProtMgmt.data_len = 2;
				}
				else
				{
					gZbProtMgmt.state = S_IDLE;
				}
				
				break;
			case S_HDR:
			{
				gZbProtMgmt.data[ gZbProtMgmt.data_len ] = data[i];
				gZbProtMgmt.data_len++;
				if( gZbProtMgmt.data_len == ZB_HDR_LEN)
				{
					gZbProtMgmt.state = S_DATA;					
				}				
				break;
			}
			case S_DATA:
			{
				gZbProtMgmt.data[ gZbProtMgmt.data_len ]  = data[i];
				gZbProtMgmt.data_len++;
				
				phdr = (struct zb_ser_frame_hdr *)gZbProtMgmt.data;
				if( phdr->ctl & BIT(3) )
				{
					
				}
				else if( gZbProtMgmt.data_len == phdr->len )
				{
						//complete pkt.
						gZbProtMgmt.state = S_CRC_BEGIN;
				}
				break;
			}
			case S_CRC_BEGIN:
			{
				gZbProtMgmt.crc[0] = data[i];
				gZbProtMgmt.state = S_CRC_END;
				break;
			}			
			case S_CRC_END:
			{
				gZbProtMgmt.crc[1] = data[i];

				//verify where crc is correct.
				crc = gen_crc(gZbProtMgmt.data, gZbProtMgmt.data_len);
				if( crc == *(zb_uint16 *)gZbProtMgmt.crc )	//little endian.
				{
					//good frame complete.
					gZbProtMgmt.good_frame_cnt++;
					ZB_DBG(">> Get a good frame. good_frame_cnt=%d\n", gZbProtMgmt.good_frame_cnt);		

					serial_frame_handle( (struct zb_ser_frame_hdr*)gZbProtMgmt.data );
				}
				else
				{
					//bad frame. drop.
					gZbProtMgmt.error_frame_cnt++;
					ZB_DBG(">> error frame.\n");	
				}
				
				gZbProtMgmt.state = S_IDLE;
				break;
			}
			default:
			{
				ZB_ERROR(">> Zigbee serial protocol state error.\n");
				break;
			}			
		}
	}
}

// 100ms
#define SERIAL_TIMEOUT_NS	1000*100

//return 0 : send success.
// 		-1 : failed to send.  timeout 3 times.
static int serial_data_output(zb_uint8 *data, zb_uint8 len)
{
	struct timespec absTime;
	int rc = 0;
	int ret = 0;
	
SEND_AGAIN:
	pthread_mutex_lock( &gZbProtMgmt.tx_mutex );

	gZbProtMgmt.tx_trans = 0;
	gZbProtMgmt.last_tx_acked = 0;

	zb_send(data,len);

	clock_gettime(CLOCK_REALTIME, &absTime);  
	
	absTime.tv_nsec += SERIAL_TIMEOUT_NS;
	absTime.tv_sec += absTime.tv_nsec/1000000000;
	absTime.tv_nsec %= 1000000000;

	//rc = 0;		
	do{
		rc = pthread_cond_timedwait( &gZbProtMgmt.tx_acked, &gZbProtMgmt.tx_mutex, &absTime);		
	}while( (rc==0) && (!gZbProtMgmt.last_tx_acked) );	//wakeup unexpected.
	
	if( rc == 0)
	{
		//waked up before timeout. success.
		ret = 0;
	}
	else
	{
		//take all error as timeout.
		gZbProtMgmt.tx_trans++;
		
		if( gZbProtMgmt.tx_trans > 3)
		{			
			ret = -1;	//Failed to send.
		}
		else
		{
			pthread_mutex_unlock( &gZbProtMgmt.tx_mutex );
			goto SEND_AGAIN;
		}		
	}

	pthread_mutex_unlock( &gZbProtMgmt.tx_mutex );

	return ret;
}

//version 1.
int zb_protocol_send(struct serial_session_node *pSNode, char *cmd, char *payload, zb_uint8 len)
{
	zb_uint16 tot_len;
	zb_uint16 crc_val;
	struct zb_ser_frame_hdr *frame;
	
	if( pSNode->tx_buf == NULL)
	{
		return -1; //no data to send.
	}

	//setup the header.
	frame = (struct zb_ser_frame_hdr *)pSNode->tx_buf;
	frame->sop_magic = 0x55AA;
	memcpy(frame->src_addr, pSNode->src_addr, ZB_ADDR_LEN);
	memcpy(frame->dst_addr, pSNode->dst_addr, ZB_ADDR_LEN);	
	frame->ctl = 0;
	++pSNode->seq;
	frame->seq = pSNode->seq;
	frame->len = len + sizeof(struct zb_ser_frame_hdr);
	tot_len = sizeof(struct zb_ser_frame_hdr);
	
	memcpy( pSNode->tx_buf + tot_len, payload, len);
	tot_len += len;

	crc_val = gen_crc(pSNode->tx_buf, tot_len);
	*(zb_uint16 *)(pSNode->tx_buf + tot_len) = crc_val;

	gZbProtMgmt.pCurTxNode = pSNode;

	return serial_data_output(pSNode->tx_buf, tot_len + 2 );	
}

//get ack from peer , wakup tx process.
static void serial_data_ack()
{
	pthread_mutex_lock(&gZbProtMgmt.tx_mutex);

	gZbProtMgmt.last_tx_acked = 1;
	pthread_cond_signal( &gZbProtMgmt.tx_acked );
	
	pthread_mutex_unlock(&gZbProtMgmt.tx_mutex);	
}


void serial_ack_frame_handle(struct serial_session_node *pSNode, struct zb_ser_frame_hdr *pFrame)
{
	if( addr_match(pFrame->src_addr, pSNode->dst_addr) && addr_match(pFrame->dst_addr, pSNode->src_addr) 
		&& (pSNode->seq == pFrame->seq) )
	{
		serial_data_ack();
	}
	else
	{
		//protocol error. ack to wrong frame.
		ZB_ERROR("-----ACK to wrong frame.\n");
	}
}

void serial_frame_handle(struct zb_ser_frame_hdr *pFrame)
{
	if( pFrame->ctl&BIT(7))
	{
		ZB_DBG(">> Get ack.\n");

		if( gZbProtMgmt.pCurTxNode != NULL)
		{
			serial_ack_frame_handle(gZbProtMgmt.pCurTxNode, pFrame);
		}
		else
		{
			ZB_ERROR(">> No Tx session, But get serial input from dev.\n");
		}	
	}
	else
	{
		ZB_DBG(">> Get data frame from dev. sending ack to it... not implied now...\n");
	}	
}


/*******************************Serial session management *************************/
struct serial_session_node *alloc_serial_session_node(void)
{
	struct serial_session_node *p;
	p = malloc( sizeof(struct serial_session_node));
	if( p == NULL)
		return p;

	memset(p, 0, sizeof(struct serial_session_node));
	INIT_LIST_HEAD(&p->list);

	return p;
}

int add_serial_session_to_list(struct serial_session_node *pNode)
{
	zb_uint16 hash;

	hash = SER_HASH(pNode->src_addr,pNode->dst_addr);
	
	list_add(&pNode->list, &gSerialSessionMgmt.list[hash]);
}

int free_serial_session(struct serial_session_node *pNode)
{
	list_del(&pNode->list);

	if( pNode->tx_buf)
	{
		free(pNode->tx_buf);		
	}
	free(pNode);
}

struct serial_session_node * find_serial_session(struct ser_addr *src, struct ser_addr *dst)
{
	zb_uint16 hash;
	struct serial_session_node *pos, *nxt;
	struct list_head *head;

	hash = SER_HASH(src->addr, dst->addr);
	head = &gSerialSessionMgmt.list[hash];

	if( list_empty(head) )
	{
		return NULL;
	}

	list_for_each_entry_safe(pos, nxt, head,list)
	{
		if( addr_match(pos->src_addr, src->addr) && addr_match(pos->dst_addr, dst->addr) )
		{
			return pos;
		}		
	}

	return NULL;
}

struct serial_session_node * find_or_add_serial_session(struct ser_addr *src, struct ser_addr *dst)
{
	struct serial_session_node *pNode;

	pNode = find_serial_session(src, dst);
	if( pNode != NULL)
	{
		return pNode;
	}

	pNode = alloc_serial_session_node();
	if( pNode == NULL)
		return pNode;

	memcpy(pNode->src_addr, src->addr, ZB_ADDR_LEN);
	memcpy(pNode->dst_addr, dst->addr, ZB_ADDR_LEN);

	//init tx buffer ?
	pNode->tx_buf = malloc(SER_TX_BUF_LEN);
	
	return pNode;
}

