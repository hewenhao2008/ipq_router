#ifndef ZB_SESSION_MGMT_H
#define ZB_SESSION_MGMT_H

#include "common_def.h"
#include "zb_serial_protocol.h"
#include "dev_mgmt.h"

struct app_endpoint
{	
	zb_uint8 logic_id;	
};

struct bs_session
{
	struct app_endpoint 	*p_app_endpoint;
	struct zigbee_endpoint  *p_zb_endpoint;

	struct serial_session_node *p_serial_sess;	
};


void test(void);

#endif

