#ifndef BIZ_MSG_PROCESS_H
#define BIZ_MSG_PROCESS_H

#include "common_def.h"
#include "kernel_list.h"

typedef enum zb_biz_status
{
	ZB_BIT_UNINITED = 0,
	ZB_BIZ_INITED   = 1,	//initialized.
	ZB_BIZ_PERMIT_JOIN_SENT = 2,
	ZB_BIZ_PERMIT_JOIN_RESPED = 3, //responsed.

	ZB_BIZ_WAIT_DEVICE_ANNOUNCE = 4,
	ZB_BIZ_GOT_DEVICE_ANNOUNCE  = 5,

	ZB_BIZ_READING_ID = 6,
	ZB_BIZ_GOT_ID = 7,

	ZB_BIZ_BIND_SENT = 8,
	ZB_BIZ_BIND_RESP = 9,

	ZB_BIZ_CONFIG_REPORTING_SENT = 10,
	ZB_BIZ_CONFIG_REPORTING_RESP = 11,	

	ZB_BIZ_SEARCH_COMPLETE = 12,
	ZB_BIZ_SEARCH_COMPLETE_DEL_IT = 13,
	
}ZB_BIZ_STATUS;

typedef enum zb_cloud_cmd
{
	C_SEARCH,	//search device.
	
}ZB_CLOUD_CMD;

struct biz_cloud_msg_info
{
	// uuid, token, xxx
	ZB_CLOUD_CMD cmd;

	zb_uint32 cmd_seq;

	zb_uint8 src_id;	//0: from APP. 1: from cloud.

	zb_uint8 dst_dev_shortid;
};

//short id + endpoint_addr

typedef struct biz_node
{
	struct list_head list;

	struct list_head search_list;

	zb_uint8 cloud_id;	//0: from APP. 1: from cloud.

	zb_uint16  dev_shortid;	//short id.
	zb_uint8   dev_endp_id;		//endpoint id.

	//devices info..
	zb_uint8    dev_mac[8];	
	
	//msg info from APP.
	struct biz_cloud_msg_info c_msg;
	
	//session info of serial session.
	struct serial_session_node *pSerialSessNode;

	//biz status.
	ZB_BIZ_STATUS 	zb_biz_status;
	int	 waitting_dev_response;
	pthread_cond_t 	cmd_acked;		//used for timeouted wait.
	pthread_mutex_t cmd_mutex;	
	
}biz_node_t;


#define BIZ_NODE_NUM	256

typedef struct biz_node_mgmt
{
	struct list_head list[BIZ_NODE_NUM];	
	
}biz_node_mgmt_t;

#define BIZ_NODE_HASH(cloud_id, shortid, endp_id)	( ((shortid>>8) ^ shortid) & 0xFF)


// function define.
int biz_node_mgmt_init(void);

biz_node_t * biz_find_node(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id);

biz_node_t * biz_find_or_add_node(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id);

int biz_search_dev(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id);


#endif
