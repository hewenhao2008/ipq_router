#ifndef ZB_SERIAL_PROTOCOL_H
#define ZB_SERIAL_PROTOCOL_H

#include "common_def.h"
#include "kernel_list.h"

#define ZB_ADDR_LEN	3
#define ZB_CMD_LEN	5

#pragma pack(1)
struct zb_ser_frame_hdr
{
	zb_uint16   sop_magic;	// 0xAA  0x55
	zb_uint8	ctl;
	zb_uint8	len;
	zb_uint8	seq;
	zb_uint8	dst_addr[ZB_ADDR_LEN];
	zb_uint8	src_addr[ZB_ADDR_LEN];
	zb_uint8	cmd[ZB_CMD_LEN];
};

struct ser_addr
{
	zb_uint8 addr[ZB_ADDR_LEN];
};

#pragma pack()



// sop_magic + ctl + len + seq + dstaddr + srcaddr + cmd
#define ZB_HDR_LEN	sizeof(struct zb_ser_frame_hdr)

#define BIT(x)	(1<<3)

#define SER_TX_BUF_LEN	(256+10)

struct serial_session_node
{
	struct list_head list;
	
	zb_uint8 src_addr[ZB_ADDR_LEN];
	zb_uint8 dst_addr[ZB_ADDR_LEN];
	zb_uint8 seq;

	zb_uint8 *tx_buf;
	zb_uint8  tx_payload_len;
};

#define SER_SESSION_HASH_NUM  128
struct serial_session_mgmt
{
	struct list_head list[SER_SESSION_HASH_NUM];
};

#define SER_HASH(psrcAddr, pdstAddr)	((psrcAddr[2]^pdstAddr[2])&0x7F)


#define addr_match(paddr1, paddr2)	((paddr1[0]==paddr2[0])&&(paddr1[1]==paddr2[1])&&(paddr1[2]==paddr2[2]))

int serial_data_in(zb_uint8 *data, zb_uint8 len);

void serial_frame_handle(struct zb_ser_frame_hdr *pFrame);

struct serial_session_node * find_or_add_serial_session(struct ser_addr *src, struct ser_addr *dst);

#endif

