#ifndef DEV_MGMT_H
#define DEV_MGMT_H

#include "common_def.h"

struct zigbee_endpoint
{
	zb_uint8 logic_id; //addr.

	zb_uint8 status;
	
};

struct zigbee_dev
{
	zb_uint16	short_id;
	zb_uint8 	mac[8];
	
	zb_uint8 	zb_endpoint_cnt; //info come from DB
	struct zigbee_endpoint endpoint[16]; //info come from DB
};

#define ZB_IEEE_ADDR_LEN	8
#define ZB_UUID_LEN			8
struct common_dev_info
{
	//64 bit IEEE addr.
	zb_uint8  ieee_addr[ZB_IEEE_ADDR_LEN];
	zb_uint16 short_addr;

	zb_uint8  dev_uuid[ZB_UUID_LEN];

	zb_uint8  status;	// 0: Not added.  1: added.
};

#define ZB_ATTR_NAME	32
struct common_dev_attr
{
	zb_uint8	attr_id;	// attr identifier
	zb_uint8    attr_type;  // data type.
	zb_uint8    attr_name[ZB_ATTR_NAME];

	zb_uint8	attr_rw_access; // Read-only/ RW	

	zb_uint8 	*attr_data;
};

int zb_dev_get_endp_cnt(zb_uint16 shortid);


#endif
