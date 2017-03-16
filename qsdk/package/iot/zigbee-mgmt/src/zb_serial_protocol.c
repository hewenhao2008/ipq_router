#include "common_def.h"
#include "zb_serial_protocol.h"
#include <pthread.h>
#include <sys/time.h>
//#include<sys/time.h>


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

static inline zb_uint8 ser_hash(zb_frame_addr_t *psrc_addr, zb_frame_addr_t *pdst_addr)
{
	return (psrc_addr->short_addr^pdst_addr->short_addr^psrc_addr->endp_addr^pdst_addr->endp_addr)&0x7F;
}


int init_zb_serial_protocol_mgmt()
{
	int i=0;

	ZB_DBG("> init_zb_serial_protocol_mgmt in.\n");
	
	memset(&gZbProtMgmt, 0, sizeof(struct zb_protocol_mgmt));
	gZbProtMgmt.state = S_IDLE;

	pthread_cond_init( &gZbProtMgmt.tx_acked, NULL );
	pthread_mutex_init( &gZbProtMgmt.tx_mutex, NULL);

	memset(&gSerialSessionMgmt, 0, sizeof(struct serial_session_mgmt));
	for(i=0; i<SER_SESSION_HASH_NUM; i++)
	{
		INIT_LIST_HEAD( &gSerialSessionMgmt.list[i] );
	}

	ZB_DBG("> init_zb_serial_protocol_mgmt out.\n");

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
		//ZB_DBG("R:%.2x S:%d   ", data[i], gZbProtMgmt.state);
		if( gZbProtMgmt.state == S_IDLE)
		{
			ZB_DBG("Rx: ");
		}
		ZB_DBG("%.2x ", data[i] );
				
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
				phdr = (struct zb_ser_frame_hdr *)gZbProtMgmt.data;
				gZbProtMgmt.data[ gZbProtMgmt.data_len ] = data[i];
				gZbProtMgmt.data_len++;
				if( gZbProtMgmt.data_len == ZB_HDR_LEN)
				{
					gZbProtMgmt.state = S_DATA;					
				}		

				//if it is ACK. no data.
				if(gZbProtMgmt.data_len == phdr->len)
				{
					//ZB_DBG(">> frame only contain header, no payload.\n");
					gZbProtMgmt.state = S_CRC_BEGIN;
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
					//ZB_DBG(">> Get a good frame. good_frame_cnt=%d crc=%x\n", gZbProtMgmt.good_frame_cnt, crc);		

					serial_frame_handle( (struct zb_ser_frame_hdr*)gZbProtMgmt.data );
				}
				else
				{
					//bad frame. drop.
					gZbProtMgmt.error_frame_cnt++;
					ZB_DBG(">> CRC error.\n");	
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



//int gettimeofday(struct  timeval*tv,struct  timezone *tz )
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
static int serial_data_output(zb_uint8 *data, zb_uint8 len)
{
	struct timespec absTime;
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

	gZbProtMgmt.tx_trans = 0;
	
SEND_AGAIN:
	pthread_mutex_lock( &gZbProtMgmt.tx_mutex );

	
	gZbProtMgmt.last_tx_acked = 0;

	print_current_time();
	
	zb_send(data,len);

	clock_gettime(CLOCK_REALTIME, &absTime);  
	
	absTime.tv_nsec += SERIAL_TIMEOUT_NS;
	absTime.tv_sec += absTime.tv_nsec/1000000000;
	absTime.tv_nsec %= 1000000000;

	//rc = 0;		
	do{
		rc = pthread_cond_timedwait( &gZbProtMgmt.tx_acked, &gZbProtMgmt.tx_mutex, &absTime);	
		if( rc == 0)
		{
			//ZB_DBG("rc=%d waked up. last_tx_acked=%d \n", rc, gZbProtMgmt.last_tx_acked);
		}
		else
		{
			ZB_DBG("rc=%d timeout. last_tx_acked=%d \n", rc, gZbProtMgmt.last_tx_acked);
		}
		
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
		
		if( gZbProtMgmt.tx_trans >= 3)
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

static int serial_data_output_direct(zb_uint8 *data, zb_uint8 len)
{	
	int ret;

	pthread_mutex_lock( &gZbProtMgmt.tx_mutex );
	
	ret = zb_send(data,len);

	pthread_mutex_unlock( &gZbProtMgmt.tx_mutex );

	return ret;
}

static void zb_record_cmd(struct serial_session_node *pSNode, char *cmd, zb_uint8 tx_seq)
{
	zb_frame_cmd_t *pcmd = (zb_frame_cmd_t *)cmd;
	zb_uint8 index = pSNode->tx_records.wr_index;
	
	pSNode->tx_records.tx_cmd_seq[index] = tx_seq;
	//pSNode->tx_records.tx_cmd[index] = (pcmd->cmd_type<<32) | (pcmd->cmd_cluster_id<<16) | pcmd->cmd_code;
	pSNode->tx_records.tx_cmd[index] = pcmd->cmd_type;
	pSNode->tx_records.tx_cmd[index] = (pSNode->tx_records.tx_cmd[index]<<32) | (pcmd->cmd_cluster_id<<16) | pcmd->cmd_code;

	pSNode->tx_records.wr_index++;
	if( pSNode->tx_records.wr_index >= BIZ_RECORD_NUM)
	{
		pSNode->tx_records.wr_index = 0;
	}
}


//version 1. 
int zb_protocol_send(struct serial_session_node *pSNode, char *cmd, char *payload, zb_uint8 len)
{
	zb_uint16 tot_len;
	zb_uint16 crc_val;
	struct zb_ser_frame_hdr *frame;
	
	if( pSNode->tx_buf == NULL)
	{
		printf(">> tx_buf is null.return.\n");
		return -1; //no data to send.
	}

	//setup the header.
	frame = (struct zb_ser_frame_hdr *)pSNode->tx_buf;
	frame->sop_magic = 0x55AA;
	memcpy( &frame->src_addr, &pSNode->src_addr, ZB_ADDR_LEN);
	memcpy( &frame->dst_addr, &pSNode->dst_addr, ZB_ADDR_LEN);	
	memcpy( &frame->cmd, cmd, sizeof(zb_frame_cmd_t));
	
	frame->ctl = 0;
	++pSNode->seq;
	frame->seq = pSNode->seq;
	frame->len = len + sizeof(struct zb_ser_frame_hdr);
	
	//tot_len = sizeof(struct zb_ser_frame_hdr);
	tot_len = ZB_HDR_LEN;

	if( len > 0)
	{
		memcpy( pSNode->tx_buf + tot_len, payload, len);
		tot_len += len;
	}
	
	crc_val = gen_crc(pSNode->tx_buf, tot_len);
	*(zb_uint16 *)(pSNode->tx_buf + tot_len) = crc_val;

	gZbProtMgmt.pCurTxNode = pSNode;

	zb_record_cmd(pSNode, cmd, pSNode->seq-1);

	#if 0
	{
		ZB_DBG("-----Serial Tx Format:---");
		ZB_DBG("-----len:%u ctl:0x%x seq:%u \n", frame->len, frame->ctl, frame->seq);
		ZB_DBG("-----(dst_shortid:0x%x endp:0x%x  src_shortid:0x%x src_endp:0x%x\n", 
						frame->dst_addr.short_addr, frame->dst_addr.endp_addr,
						frame->src_addr.short_addr, frame->src_addr.endp_addr);
		ZB_DBG("-----cmd: Type:0x%x clusterid:0x%x code:0x%x \n", 
						frame->cmd.cmd_type, frame->cmd.cmd_cluster_id, frame->cmd.cmd_code);
	}
	#endif
	
	return serial_data_output(pSNode->tx_buf, tot_len + 2 );	
}

int zb_protocol_send_ack(char *data, zb_uint8 len)
{
	//send without waitting.
	serial_data_output_direct(data, len);
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
	if( addr_match(&pFrame->src_addr, &pSNode->dst_addr) && addr_match(&pFrame->dst_addr, &pSNode->src_addr) 
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

static zb_uint8 ack_frame[256];
int serial_reply_frame(struct zb_ser_frame_hdr *pFrame)
{	
	struct zb_ser_frame_hdr* pack = (struct zb_ser_frame_hdr*)ack_frame;
	zb_uint16 *pcrc;

	//build ack frame.
	pack->sop_magic = SOP_MAGIC_CODE;
	pack->len = ZB_HDR_LEN;
	pack->ctl = BIT(7);
	pack->seq = pFrame->seq;
	memcpy( &pack->dst_addr,  &pFrame->src_addr, ZB_ADDR_LEN);
	memcpy( &pack->src_addr,  &pFrame->dst_addr, ZB_ADDR_LEN);
	memcpy( &pack->cmd,  &pFrame->cmd, ZB_CMD_LEN);

	pcrc = (zb_uint16 *)((zb_uint8 *)pack + ZB_HDR_LEN);
	*pcrc = gen_crc( (zb_uint8 *)pack, ZB_HDR_LEN);

	return zb_protocol_send_ack( (zb_uint8 *)pack, ZB_HDR_LEN + 2);
}



biz_msg_handle_cb func_biz_msg_handle = NULL;

void register_biz_msg_handle(biz_msg_handle_cb func)
{
	func_biz_msg_handle = func;
}

void unregister_biz_msg_handle()
{
	func_biz_msg_handle = NULL;
}


int session_layer_handle_rx(struct zb_ser_frame_hdr *pFrame)
{
	//find session Node. switch dst and src.	
	struct serial_session_node * pnode;

	//switch dst addr and src addr.
	pnode = find_or_add_serial_session(&pFrame->dst_addr, &pFrame->src_addr);
	if( pnode == NULL)
	{
		ZB_ERROR(">> Failed to handle msg.\n");
		return -1;
	}

	pnode->rx_seq = pFrame->seq;

	if(func_biz_msg_handle)
	{
		func_biz_msg_handle(pFrame, pnode);
	}
	
}
	

void serial_frame_handle(struct zb_ser_frame_hdr *pFrame)
{
	if( pFrame->ctl&BIT(7))
	{		
		if( gZbProtMgmt.pCurTxNode != NULL)
		{			
			ZB_DBG("<---- Rx Ack. expected.\n");
			serial_ack_frame_handle(gZbProtMgmt.pCurTxNode, pFrame);
		}
		else
		{
			ZB_ERROR("<---- Rx ACK. No Tx session, But get serial input from dev.\n");
		}	
	}
	else
	{
		ZB_DBG("<---- Rx data frame. ACK it now...\n");
		
		serial_reply_frame(pFrame);

		//session layer handle this frame.
		session_layer_handle_rx(pFrame);
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

	hash = ser_hash(&pNode->src_addr, &pNode->dst_addr);
	
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

struct serial_session_node * find_serial_session(ser_addr_t *src, ser_addr_t *dst)
{
	zb_uint16 hash;
	struct serial_session_node *pos, *nxt;
	struct list_head *head;

	hash = ser_hash(src, dst);
	head = &gSerialSessionMgmt.list[hash];

	if( list_empty(head) )
	{
		return NULL;
	}

	list_for_each_entry_safe(pos, nxt, head,list)
	{
		if( addr_match(&pos->src_addr, src) && addr_match(&pos->dst_addr, dst) )
		{
			return pos;
		}		
	}

	return NULL;
}

struct serial_session_node * find_or_add_serial_session(ser_addr_t *src, ser_addr_t *dst)
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

	memcpy( &pNode->src_addr, src, ZB_ADDR_LEN);
	memcpy( &pNode->dst_addr, dst, ZB_ADDR_LEN);

	//init tx buffer ?
	pNode->tx_buf = malloc(SER_TX_BUF_LEN);

	//debug. set default seq to 0xAA.
	pNode->seq = 0xAA;
	
	add_serial_session_to_list(pNode);
	
	return pNode;
}

