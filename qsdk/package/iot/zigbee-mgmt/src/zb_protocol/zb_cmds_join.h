#ifndef ZB_CMDS_JOIN_H
#define ZB_CMDS_JOIN_H

#include "common_def.h"
#include "zb_serial_protocol.h"
#include "biz_msg_process.h"

int coord_msg_handle_dev_join(struct zb_ser_frame_hdr *pFrame);

int host_send_cmd_permit_join(biz_node_t *pBizNode);

int host_send_get_ids(struct serial_session_node *pNode);

int host_send_bind_request(struct serial_session_node *pNode, zb_uint8 *dst_mac, 
									  zb_uint8 dst_endp,zb_uint16 cluster_id);

int host_send_rpt_configure(struct serial_session_node *pNode );

#endif


