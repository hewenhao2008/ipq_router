#ifndef ZB_CMDS_H
#define ZB_CMDS_H

#include "biz_msg_process.h"
#include "zb_serial_protocol.h"

struct cmd_cont
{
	int index;
	zb_frame_cmd_t cmd;
	zb_uint8	payload[256];
	zb_uint8 	payload_len;
};


enum cmd_to_coord
{
	CT_SEARCH_DEV 	= 0, 	//搜索设备 
	CT_DEFAULT_RESP = 1,
	CT_DEL_DEV 		= 2,
	CT_WRITE_ATTR	= 3,
	CT_READ_ATTR 	= 4,
	CT_BIND_REQ 	= 5,
	CT_RPT_CONFIG 	= 6,
	CT_LEAVE_REQ 	= 7,
	CT_CMD_ON_OFF 	= 8,	
	
	CT_LIGHT_MOVE_TO_COLOR = 9,
	CT_LIGHT_MOVE_TO_COLOR_TEMPERATURE= 10,
	CT_LIGHT_MOVE_TO_LEVEL = 11,
	
};

enum cmd_from_coord
{
	CF_DEFAULT_RESP = 0,
	CF_JOIN_INFO_RPT = 1, 	// 入网信息上报	
};


enum CMD_TYPE
{
	CMD_READ_ATTR = 0x00,
	CMD_WRITE_ATTR = 0x02,
	CMD_DEFAULT_RESP = 0x0B, // 默认回复
	CMD_COMMAND = 0x0E,	// 控制	
};


int init_session_with_coord(void);

int zb_dev_get_id(biz_node_t *pBizNode);

int zb_dev_bind_request(biz_node_t *pBizNode, zb_uint8 *dst_mac, zb_uint8 dst_endp,zb_uint16 cluster_id);

ser_addr_t * host_get_hostaddr(void);

struct cmd_cont * coord_get_global_cmd(zb_uint8 index);

struct serial_session_node * host_get_session_with_coord();

void send_cmd_to_coord(zb_uint8 *cmd, zb_uint8 *payload , zb_uint8 payload_len);

//common commands.
int host_send_dev_leave_request(struct serial_session_node *pNode, zb_uint8 *dst_mac, zb_uint8 option);



#endif

