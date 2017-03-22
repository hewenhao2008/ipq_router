#include "common_def.h"
#include "zb_serial_protocol.h"
#include <pthread.h>
#include <sys/time.h>
#include "rdt_tx_thread.h"
//#include<sys/time.h>

#include "biz_msg_process.h"	//JOIN status.

typedef enum e_ZBP_STATE
{
	S_IDLE,		//idle, waitting for start of frame.
	S_SOP_BEGIN,	//get 0x55
	
	S_HDR,		//recving data, till all S_BODY received.
	S_DATA,
	S_CRC_BEGIN,
	S_CRC_END,
	
}ZBP_STATE;

struct zb_frame_mgmt
{
	ZBP_STATE state;
	
	zb_uint8	data[SBUF_LEN];
	int 		data_len;
	zb_uint8	crc[2];

	//debug cnt.
	zb_uint32 	good_frame_cnt;
	zb_uint32   error_frame_cnt;	
};

struct zb_frame_mgmt	gZFrameMgmt;

struct serial_session_mgmt gSerialSessionMgmt;

struct list_head join_list;

//use global sequence no to trace the command sent by host.
zb_uint8 gSeqJoinPhase = 0;
zb_uint8 gSeqWorking = SEQ_DEV_JOIN_PHASE_MAX;

//Let all endpoint of a device have the same hash.
static inline zb_uint8 ser_hash(zb_frame_addr_t *psrc_addr, zb_frame_addr_t *pdst_addr)
{
	return (psrc_addr->short_addr^pdst_addr->short_addr)&0x7F;
}

zb_uint8 get_new_seq_no(int join_phase)
{
	if( join_phase)
	{
		++gSeqJoinPhase;
		if( gSeqJoinPhase >= SEQ_DEV_JOIN_PHASE_MAX)
		{
			gSeqJoinPhase = 0;
		}
		return gSeqJoinPhase;
	}
	else
	{
		++gSeqWorking;
		if( gSeqWorking >= 255)
		{
			gSeqWorking = SEQ_DEV_JOIN_PHASE_MAX;
		}
		return gSeqWorking;
	}	
}

int init_zb_serial_protocol_mgmt()
{
	int i=0;

	ZB_DBG("> init_zb_serial_protocol_mgmt in.\n");
	
	memset(&gZFrameMgmt, 0, sizeof(struct zb_frame_mgmt));
	gZFrameMgmt.state = S_IDLE;

	memset(&gSerialSessionMgmt, 0, sizeof(struct serial_session_mgmt));
	for(i=0; i<SER_SESSION_HASH_NUM; i++)
	{
		INIT_LIST_HEAD( &gSerialSessionMgmt.list[i] );
	}

	INIT_LIST_HEAD(&join_list);

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
		if( gZFrameMgmt.state == S_IDLE)
		{
			ZB_DBG("Rx: ");
		}
		ZB_DBG("%.2x ", data[i] );
				
		switch( gZFrameMgmt.state )
		{
			case S_IDLE:
				if( data[i] == 0xAA)
				{
					gZFrameMgmt.state = S_SOP_BEGIN;
				}				
				break;
			case S_SOP_BEGIN:
				if( data[i] == 0x55 )
				{
					gZFrameMgmt.state = S_HDR;
					gZFrameMgmt.data[0] = 0xAA;
					gZFrameMgmt.data[1] = 0x55;
					gZFrameMgmt.data_len = 2;
				}
				else
				{
					gZFrameMgmt.state = S_IDLE;
				}
				
				break;
			case S_HDR:
			{
				phdr = (struct zb_ser_frame_hdr *)gZFrameMgmt.data;
				gZFrameMgmt.data[ gZFrameMgmt.data_len ] = data[i];
				gZFrameMgmt.data_len++;
				if( gZFrameMgmt.data_len == ZB_HDR_LEN)
				{
					gZFrameMgmt.state = S_DATA;					
				}		

				//if it is ACK. no data.
				if(gZFrameMgmt.data_len == phdr->len)
				{
					//ZB_DBG(">> frame only contain header, no payload.\n");
					gZFrameMgmt.state = S_CRC_BEGIN;
				}
				break;
			}
			case S_DATA:
			{
				gZFrameMgmt.data[ gZFrameMgmt.data_len ]  = data[i];
				gZFrameMgmt.data_len++;
				
				phdr = (struct zb_ser_frame_hdr *)gZFrameMgmt.data;
				if( phdr->ctl & BIT(3) )
				{
					
				}
				else if( gZFrameMgmt.data_len == phdr->len )
				{
						//complete pkt.
						gZFrameMgmt.state = S_CRC_BEGIN;
				}
				break;
			}
			case S_CRC_BEGIN:
			{
				gZFrameMgmt.crc[0] = data[i];
				gZFrameMgmt.state = S_CRC_END;
				break;
			}			
			case S_CRC_END:
			{
				gZFrameMgmt.crc[1] = data[i];

				//verify where crc is correct.
				crc = gen_crc(gZFrameMgmt.data, gZFrameMgmt.data_len);
				if( crc == *(zb_uint16 *)gZFrameMgmt.crc )	//little endian.
				{
					//good frame complete.
					gZFrameMgmt.good_frame_cnt++;
					//ZB_DBG(">> Get a good frame. good_frame_cnt=%d crc=%x\n", gZbProtMgmt.good_frame_cnt, crc);		

					zframe_handle( (struct zb_ser_frame_hdr*)gZFrameMgmt.data );
				}
				else
				{
					//bad frame. drop.
					gZFrameMgmt.error_frame_cnt++;
					ZB_DBG(">> CRC error.\n");	
				}
				
				gZFrameMgmt.state = S_IDLE;
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

int serial_data_output(zb_uint8 *data, zb_uint8 len)
{
	serial_tx_buf_t *pbuf;
	pbuf = malloc( sizeof(serial_tx_buf_t) );
	if( pbuf == NULL)
		return -1;
	
	pbuf->dat = malloc( len );
	if( pbuf->dat == NULL)
	{
		free( pbuf);
		return -1;
	}
	
	memcpy( pbuf->dat, data, len);
	pbuf->dat_len = len;
	pbuf->need_ack = 1;
	
	rdt_send(pbuf);
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
	frame->seq = pSNode->seq;
	frame->len = len + sizeof(struct zb_ser_frame_hdr);	
	tot_len = ZB_HDR_LEN;	//tot_len = sizeof(struct zb_ser_frame_hdr);
	if( len > 0)
	{
		memcpy( pSNode->tx_buf + tot_len, payload, len);
		tot_len += len;
	}
	
	crc_val = gen_crc(pSNode->tx_buf, tot_len);
	*(zb_uint16 *)(pSNode->tx_buf + tot_len) = crc_val;

	//gZbProtMgmt.pCurTxNode = pSNode;	
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
	//serial_data_output_direct(data, len);

	serial_tx_buf_t *pbuf;
	pbuf = malloc( sizeof(serial_tx_buf_t) );
	if( pbuf == NULL)
		return -1;
	
	pbuf->dat = malloc( len );
	if( pbuf->dat == NULL)
	{
		free( pbuf);
		return -1;
	}
	
	memcpy( pbuf->dat, data, len);
	pbuf->dat_len = len;
	pbuf->need_ack = 0;
	
	rdt_send(pbuf);
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
	if(func_biz_msg_handle)
	{
		func_biz_msg_handle(pFrame);
	}	
}
	

void zframe_handle(struct zb_ser_frame_hdr *pFrame)
{
	if( pFrame->ctl&BIT(7))
	{		
		if( is_ack_to_last_data(pFrame->seq) )
		{
			rdt_tx_data_acked();
		}
		else
		{
			ZB_ERROR("<---- Rx ACK.Seq not match.\n");
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
	//INIT_LIST_HEAD(&p->chain_for_adding);
	INIT_LIST_HEAD(&p->search_list);

	return p;
}

int add_serial_session_to_work_list(struct serial_session_node *pNode)
{
	zb_uint16 hash;

	hash = ser_hash(&pNode->src_addr, &pNode->dst_addr);
	
	list_add(&pNode->list, &gSerialSessionMgmt.list[hash]);

	ZB_DBG("\n\n>>>> ADD dev to work list. short_id=%x endp:%x <<<<\n", 	
			pNode->dst_addr.short_addr, pNode->dst_addr.endp_addr );
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

// called when we kick a device in working phase.
int dev_delete_work_phase(struct serial_session_node *pEndpNode)
{
	zb_uint16 hash;
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	int found=0;

	//All endpoint has the same hash. so we didn't need to scan the whole list.
	hash = ser_hash(&pEndpNode->src_addr, &pEndpNode->dst_addr);
	head = &gSerialSessionMgmt.list[hash];

	if( list_empty(head) )
	{
		return -1;
	}

	while(1)
	{
		found = 0;
		list_for_each_entry_safe(pos, nxt, head,list)
		{
			if( pos->dst_addr.short_addr == pEndpNode->dst_addr.short_addr )
			{
				found = 1;
				break;
			}		
		}

		if( found)
		{
			list_del( &pos->list );
			
			if( pos->tx_buf)
			{
				free(pos->tx_buf);		
			}
			free(pos);
		}
		else
		{
			break;
		}		
	}	

	return 0;
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
	
	add_serial_session_to_work_list(pNode);
	
	return pNode;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
struct serial_session_node * find_endp_from_join_list(ser_addr_t *addr, zb_uint8 seq)
{
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	
	head = &join_list;

	if( list_empty(head) )
	{
		return NULL;
	}

	list_for_each_entry_safe(pos, nxt, head,list)
	{
		if( (pos->dst_addr.short_addr == addr->short_addr) && (pos->dst_addr.endp_addr == addr->endp_addr) )
		{
			return pos;
		}		
	}

	return NULL;
}

struct serial_session_node * find_endp_from_join_list_byseq(zb_uint16 short_addr, zb_uint8 seq)
{
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	
	head = &join_list;

	if( list_empty(head) )
	{
		return NULL;
	}

	ZB_DBG(">> Find endp by seq + shortaddr, short_addr:%x seq:%x \n", short_addr, seq);
	list_for_each_entry_safe(pos, nxt, head,list)
	{
		if( (pos->dst_addr.short_addr == short_addr) && (pos->seq == seq) )
		{
			return pos;
		}		
		else
		{
			ZB_DBG(">> pos in list: short_addr:%x seq:%x\n", pos->dst_addr.short_addr, pos->seq);
		}
	}

	return NULL;
}


struct serial_session_node * find_dev_from_join_list(ser_addr_t *addr)
{
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	
	head = &join_list;

	if( list_empty(head) )
	{
		return NULL;
	}

	list_for_each_entry_safe(pos, nxt, head,list)
	{
		if( (pos->dst_addr.short_addr == addr->short_addr) && (pos->dst_addr.endp_addr == 0) )
		{
			return pos;
		}		
	}

	return NULL;
}

struct serial_session_node * add_dev_to_join_list(ser_addr_t *src, ser_addr_t *dst)
{
	struct serial_session_node * pNode ;

	pNode = alloc_serial_session_node();
	if(pNode == NULL)
	{
		ZB_ERROR("!!! Failed to allocate session node.!!!\n");
		return NULL;
	}

	memcpy( &pNode->src_addr, src, ZB_ADDR_LEN);
	memcpy( &pNode->dst_addr, dst, ZB_ADDR_LEN);

	//init tx buffer ?
	pNode->tx_buf = malloc(SER_TX_BUF_LEN);
	
	list_add( &pNode->list, &join_list);

	return pNode;
}

struct serial_session_node * add_endp_to_join_list(ser_addr_t *src, ser_addr_t *dst)
{
	return add_dev_to_join_list(src, dst);
}

//Verify whether all endpoint on a device joined success.
static int all_endp_join_success(zb_uint16 short_addr)
{
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	int has_fail = 0;
	
	head = &join_list;

	if( list_empty(head) )
	{
		ZB_DBG("--- join_list is empty ---\n");
		return 0;
	}
	
	list_for_each_entry_safe(pos, nxt, head,list)
	{
		ZB_DBG("---pos:%p pos->zb_join_status=%d pos->dst_addr.short_addr=%x endp:%x [obj short_addr:%x]--\n", 
							pos, pos->zb_join_status, pos->dst_addr.short_addr, 
							pos->dst_addr.endp_addr, short_addr );
							
		//not counter endp0. ZDO
		if( (pos->dst_addr.short_addr == short_addr) && (pos->dst_addr.endp_addr != 0) )
		{
			if( pos->zb_join_status < ZB_BIZ_SEARCH_COMPLETE)
			{
				has_fail = 1;	
				ZB_DBG("--- pos->zb_join_status=%d pos->dst_addr.short_addr=%x---\n", 
							pos->zb_join_status, pos->dst_addr.short_addr );
			}			
		}		
	}

	return !has_fail;
		
}

// if all device complete the join steps. move it to work list.
int dev_complete_join(zb_uint16 short_addr)
{
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	int found = 0;
	int dbgcnt = 0;
	
	ZB_DBG(">> dev_complete_join, Trying short_addr=%x\n", short_addr);
	
	if( !all_endp_join_success(short_addr) )
	{
		ZB_DBG(">> !!! NOT all enpd on this device has complete the join.\n");
		return 0;
	}

	head = &join_list;
	
	do
	{
		found = 0;
		
		list_for_each_entry_safe(pos, nxt, head,list)
		{
			if(pos->dst_addr.short_addr == short_addr)
			{
				found = 1;
				break;
			}
		}

		if(found)
		{
			//remove from join_list
			list_del( &pos->list );

			if( pos->dst_addr.endp_addr == 0) //This is ZDO
			{
				//Reply to app by Json
			}

			//add to normal working list.
			add_serial_session_to_work_list( pos );		
			dbgcnt++;
		}
		else
		{
			ZB_DBG("!!! complete endpoint counter is : %d\n", dbgcnt);
			break;
		}		
	}while(1);
	
}


//when we kick device, remove it from the join list
int dev_delete_join_phase(zb_uint16 short_addr)
{
	int found ;
	struct serial_session_node *pos, *nxt;
	struct list_head *head;
	int hash = 0;
	
	head = &join_list;			
	do
	{
		found = 0;		
		list_for_each_entry_safe(pos, nxt, head,list)
		{
			if(pos->dst_addr.short_addr == short_addr)
			{
				found = 1;
				break;
			}
		}
		
		if(found)
		{
			//remove from join_list.
			ZB_DBG("!!! delete pos:%p short_addr:%x endp:%x !!!\n", 
						pos, pos->dst_addr.short_addr, pos->dst_addr.endp_addr);
			
			list_del( &pos->list );
			pos->pBizNodeDoingAdding = NULL;
			if( pos->tx_buf ) {
				free(pos->tx_buf);
			}
			free(pos);					
		}
		else
		{
			ZB_DBG("!!! delete complete. short_addr:%x !!!\n", short_addr);
			break;
		}		
	}while(1);	
	
}

