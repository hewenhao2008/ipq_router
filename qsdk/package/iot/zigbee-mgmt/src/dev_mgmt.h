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
	zb_uint8 mac[6];

	zb_uint8 zigbee_endpoint_cnt;
	
	struct zigbee_endpoint endpoint[16];	
};

#endif
