#ifndef ZB_SERIAL_PROTOCOL_H
#define ZB_SERIAL_PROTOCOL_H

#include "common_def.h"
#include "kernel_list.h"



#pragma pack(1)

typedef struct zb_frame_addr
{
	zb_uint16	short_addr;	//short addr
	zb_uint8	endp_addr;		//endpoint addr.
}zb_frame_addr_t;

typedef struct zb_frame_cmd
{
	zb_uint8   cmd_type;		 //00: read attr  1: write attr ...
	zb_uint16  cmd_cluster_id; // 5: switch 0x300 color
	zb_uint16  cmd_code;		// on/off/toggle.
}zb_frame_cmd_t;

//#define ZB_ADDR_LEN	3
#define ZB_ADDR_LEN		sizeof(zb_frame_addr_t)

//#define ZB_CMD_LEN	5
#define ZB_CMD_LEN		sizeof(zb_frame_cmd_t)

#define SOP_MAGIC_CODE 0x55AA
//
//帧头	数据长度	帧控制域	序列号	目的地址	源地址	命令	保留字段	数据	帧检验码
//2Bytes 1 Bytes	1Bytes	     1Byte	 3Bytes	     3Bytes	5Bytes	0～NBytes	0~65535 Bytes	2Byte(CRC)
//0x55aa									

struct zb_ser_frame_hdr
{
	zb_uint16   sop_magic;	// 0xAA  0x55
	zb_uint8	len;	//Length from 'Len' -> CRC. not include sop_magic. include CRRC.
	zb_uint8	ctl;
	zb_uint8	seq;
	
	zb_frame_addr_t	dst_addr;
	zb_frame_addr_t src_addr;
	zb_frame_cmd_t 	cmd;
	
	//zb_uint8	dst_addr[ZB_ADDR_LEN];
	//zb_uint8	src_addr[ZB_ADDR_LEN];
	//zb_uint8	cmd[ZB_CMD_LEN];
};

typedef zb_frame_addr_t ser_addr_t;

//struct ser_addr
//{
//	zb_uint8 addr[ZB_ADDR_LEN];
//};

#pragma pack()

// sop_magic  + len + ctl + seq + dstaddr + srcaddr + cmd
#define ZB_HDR_LEN	(sizeof(struct zb_ser_frame_hdr))

#define BIT(x)	(1<<x)

#define SER_TX_BUF_LEN	(256+10)

#define BIZ_RECORD_NUM 8
struct biz_record
{
	zb_uint8 	tx_cmd_seq[BIZ_RECORD_NUM];
	zb_uint64 	tx_cmd[BIZ_RECORD_NUM];
	zb_uint8  	tx_cmd_type;

	zb_uint8 wr_index;
};

struct serial_session_node
{
	struct list_head list;

	zb_frame_addr_t	src_addr;
	zb_frame_addr_t dst_addr;
	
	zb_uint8 seq;

	zb_uint8 *tx_buf;
	zb_uint8  tx_payload_len;
	zb_uint8  biz_cmd_status;

	struct biz_record  tx_records;

	zb_uint8 rx_seq;	
};

#define SER_SESSION_HASH_NUM  128
struct serial_session_mgmt
{
	struct list_head list[SER_SESSION_HASH_NUM];
};


//#define SER_HASH(psrcAddr, pdstAddr)	((psrcAddr->short_addr^pdstAddr->short_addr^psrcAddr->endp_addr^pdstAddr->endp_addr)&0x7F)

//#define addr_match(paddr1, paddr2)	((paddr1[0]==paddr2[0])&&(paddr1[1]==paddr2[1])&&(paddr1[2]==paddr2[2]))
#define addr_match(paddr1, paddr2)		( !memcmp(paddr1, paddr2, ZB_ADDR_LEN) )


int serial_data_in(zb_uint8 *data, zb_uint8 len);

void serial_frame_handle(struct zb_ser_frame_hdr *pFrame);

struct serial_session_node * find_or_add_serial_session(ser_addr_t *src, ser_addr_t *dst);


typedef int (*biz_msg_handle_cb)(struct zb_ser_frame_hdr *pFrame, struct serial_session_node *pnode);

void register_biz_msg_handle(biz_msg_handle_cb func);

#endif

