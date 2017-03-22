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

	zb_uint8 dst_dev_short_addr;
	zb_uint8 dst_dev_endp_addr;
	
	zb_uint8 src_id;	//Not used. 0: from APP. 1: from cloud.	
};

//short id + endpoint_addr
#define BIZ_MANUFACTURE_NAME_LEN	33
#define BIZ_MODULE_ID_LEN	33

typedef struct biz_node
{
	struct list_head list;

	struct list_head search_list;

	zb_uint8 cloud_id;	//0: from APP. 1: from cloud.

	zb_uint16  dev_shortid;		//short id.
	zb_uint8   dev_endp_id;		//endpoint id. 

	//devices info... 
	zb_uint8    dev_mac[8];	
	
	//msg info from APP.
	zb_uint8 token[64];
	zb_uint8 uuid[64];
	struct biz_cloud_msg_info c_msg;
	
	//session info of serial session.
	//Main session.'permit join cmd' : point to coord
	//				other command    : point to endp.
	struct serial_session_node *pSerialSessNode; 

	//Only used for adding device during 'permit join' serial_session_node*
	struct list_head endp_chain; 

	char manufactureName[BIZ_MANUFACTURE_NAME_LEN];
	char moduleId[BIZ_MODULE_ID_LEN];
	zb_uint8 endpoint_cnt;
	zb_uint8 endpoint_addrs[256];
	
	//biz status....
	//different device may have different status. map short_Id to status.
	//ZB_BIZ_STATUS 	zb_biz_status[256];	//each short_id can have one status.
	//int  zb_biz_status[256];	
	int	 waitting_dev_response;	
	
}biz_node_t;


#define BIZ_NODE_NUM	256

typedef struct biz_node_mgmt
{
	struct list_head list[BIZ_NODE_NUM];	

	biz_node_t *pg_biz_node_adding_dev;	//The only one biz node doing device searching and adding.
	
}biz_node_mgmt_t;

//#define BIZ_NODE_HASH(cloud_id, shortid, endp_id)	( ((shortid>>8) ^ shortid) & 0xFF)
#define BIZ_NODE_HASH(token)	( (token[0]) & 0xFF)



// function define.
int biz_node_mgmt_init(void);

biz_node_t * biz_find_node(char * token);

biz_node_t * biz_find_or_add_node(char * token);

int biz_search_dev(char * token, zb_uint8 *uuid, char * manufactureName, char *moduleId);


biz_node_t * biz_get_node_doing_search();

void biz_set_node_doing_search(biz_node_t * pNode);


#endif
