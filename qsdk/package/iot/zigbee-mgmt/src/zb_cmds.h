#ifndef ZB_CMDS_H
#define ZB_CMDS_H

#include "biz_msg_process.h"

int init_session_with_coord(void);

int zb_dev_get_id(biz_node_t *pBizNode);

int zb_dev_bind_request(biz_node_t *pBizNode, zb_uint8 *dst_mac, zb_uint8 dst_endp,zb_uint16 cluster_id);


#endif

