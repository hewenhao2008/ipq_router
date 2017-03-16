#include "common_def.h"
#include "zb_session_mgmt.h"
#include "zb_serial_protocol.h"

#include "biz_msg_process.h"
#include "zb_data_type.h"

int coord_msg_handle(struct zb_ser_frame_hdr *pFrame, struct serial_session_node *pnode);


#define HOST_ADDR_ENDP_ADDR 0x1
#define COORD_ADDR_ENDP_ADDR 0xF1

ser_addr_t host_addr = { .short_addr=0, 
					.endp_addr= HOST_ADDR_ENDP_ADDR };
					
ser_addr_t coord_addr = { .short_addr=0, 
						 .endp_addr= COORD_ADDR_ENDP_ADDR };;

//the session node to communicate with coordinate.
struct serial_session_node * gSessWithCoord;

int init_session_with_coord()
{
	gSessWithCoord = find_or_add_serial_session(&host_addr, &coord_addr);
	if( gSessWithCoord == NULL)
	{
		ZB_ERROR(">>> FATAL error. Failed to create session to coord.\n");
		return -1;
	}

	register_biz_msg_handle(coord_msg_handle);
	
	return 0;
}

//dst addr is coord.
void send_cmd_to_coord(zb_uint8 *cmd, zb_uint8 *payload , zb_uint8 payload_len)
{	
	int ret;
	
	ret = zb_protocol_send(gSessWithCoord, cmd, payload, payload_len);
	if( ret  == 0)
	{
		//printf(">> send success.\n");
	}
	else
	{
		printf(">> send fail.\n");
	}
}

//dst addr is device.
void send_cmd_to_dev(biz_node_t *pBizNode, zb_uint8 *cmd, zb_uint8 *payload , zb_uint8 payload_len)
{	
	int ret;
	
	ret = zb_protocol_send(pBizNode->pSerialSessNode, cmd, payload, payload_len);
	if( ret  == 0)
	{
		//printf(">> send success.\n");
	}
	else
	{
		printf(">> send fail.\n");
	}
}



enum cmd_to_coord
{
	CT_SEARCH_DEV = 0, 	//搜索设备 
	CT_DEFAULT_RESP = 1,
	CT_DEL_DEV = 2,

	CT_WRITE_ATTR=3,
	CT_READ_ATTR =4,

	CT_BIND_REQ = 5,

	CT_RPT_CONFIG = 6,

	CT_LEAVE_REQ = 7,

	CT_CMD_ON_OFF = 8,
	
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

struct cmd_cont
{
	int index;
	zb_frame_cmd_t cmd;
	zb_uint8	payload[256];
	zb_uint8 	payload_len;
};


struct cmd_cont gCmds_to_coord[] = {

	/**** C_SEARCH_DEV *****/
	{
		.index = CT_SEARCH_DEV,
		.cmd = {
					.cmd_type=0x40, 
			 		.cmd_cluster_id=0x0036, 
			 		.cmd_code=0
				},
	  	.payload = {0x78, 0x00},	// default 120s.
	  	.payload_len = 2
	},

	/**** CF_JOIN_INFO_RPT ****/
	{
		.index = CF_JOIN_INFO_RPT,
		.cmd = {
					.cmd_type=0x0E, 
			 		.cmd_cluster_id=0xFE00, 
			 		.cmd_code=0x01
				},
	  	.payload = {0}	// dev info.
	},

	/**** CT_DEL_DEV ****/ 
	{
		.index = CT_DEL_DEV,
	},

	/******* CT_WRITE_ATTR *************/
	{
		.index = CT_WRITE_ATTR,
		.cmd = {
					.cmd_type=CMD_WRITE_ATTR,  //02
			 		.cmd_cluster_id=0x0006,	//ON/OFF 
			 		.cmd_code=0
				},
	  	.payload = {0}	// dev info.
	},

	/***** CT_READ_ATTR **************/
	{
		.index = CT_READ_ATTR,
		.cmd = {
					.cmd_type= 0,  //00
			 		.cmd_cluster_id=0,	//decide by attr.
			 		.cmd_code=0x04
				},
	  	.payload = {0}	// dev info.
	},

	/********CT_BIND_REQ ***************/
	{
		.index = CT_BIND_REQ,
		.cmd = {
					.cmd_type= 0x40,  
			 		.cmd_cluster_id=0x21,	
			 		.cmd_code=0x0
				},
	  	.payload = {0}	// dst_mac+dst_endp_clusterid
	},

	/**********CT_RPT_CONFIG********************/
	{		
		.index = CT_RPT_CONFIG,
		.cmd = {
					.cmd_type= 0x06,  
			 		.cmd_cluster_id=0x00, //depends on attr	
			 		.cmd_code=0xC0DE	  //Not used.
				},
	  	.payload = {0}	
	},

	/**** CT_LEAVE_REQ **************/
	{
		.index = CT_LEAVE_REQ,
		.cmd = {
					.cmd_type= 0x40,  
			 		.cmd_cluster_id=0x0034,	
			 		.cmd_code=0x1
				},
	  	.payload = {0}	
	},

	/**** CT_CMD_ON_OFF **************/
	{
		.index = CT_CMD_ON_OFF,
		.cmd = {
					.cmd_type= 0x41,  
			 		.cmd_cluster_id=0x0006,	
			 		.cmd_code=0x2
				},
	  	.payload = {0}	
	}
};

struct cmd_cont gCmds_from_coord[] = {

	/**** CF_DEFAULT_RESP *****/
	{
		.index = CF_DEFAULT_RESP,
		.cmd = {
					.cmd_type=0x0B, 
			 		.cmd_cluster_id=0xFE00, 
			 		.cmd_code=0
				},
	  	.payload = {0x00}	// status
	},

	
	
};


/////////////////////////////////////////////////////////////////////////////
// search device. allow them to join network.
int coord_search_dev(biz_node_t *pBizNode)
{
	struct cmd_cont *pcmd;

	pcmd = &gCmds_to_coord[CT_SEARCH_DEV];

	pBizNode->pSerialSessNode = gSessWithCoord;

	ZB_DBG("--------coord_search_dev(): Search Device Permit Join --------\n");
	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len );

	//coord should reply with default response.	
	pBizNode->zb_biz_status = ZB_BIZ_PERMIT_JOIN_SENT;
	
	return 0;
}

int coord_get_attr_single(biz_node_t *pBizNode, zb_uint16 attr_custer_id, zb_uint16 attr_code)
{
	struct cmd_cont *pcmd;

	pcmd = &gCmds_to_coord[CT_READ_ATTR];
	pcmd->cmd.cmd_cluster_id = attr_custer_id;
	pcmd->cmd.cmd_code = attr_code;

	pBizNode->pSerialSessNode = gSessWithCoord;
	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len );
}

#if 0
int coord_get_id(biz_node_t *pBizNode)
{
	char *attr_payload;
	struct cmd_cont *pcmd;
	struct zb_frame_cmd_t *pcmd2;
	
	pcmd = &gCmds_to_coord[CT_READ_ATTR];
	pcmd->cmd.cmd_cluster_id = 0x0;
	pcmd->cmd.cmd_code = 0x0004;	//Manufacturer id.

	//pcmd2 = (struct zb_frame_cmd_t *)pcmd->payload -1 ; //back skip the cmd_type.
	//pcmd2->cmd.cmd_cluster_id = 0x0;	//module id.
	//pcmd2->cmd.cmd_code = 0x05;
	*(zb_uint16 *)pcmd->payload = 0;
	*(zb_uint16 *)(pcmd->payload + 2) = 0x5;
	
	pcmd->payload_len = 4;
	
	send_cmd_to_coord( (zb_uint8 *)pcmd, pcmd->payload, pcmd->payload_len);
	
}
#endif

//send command to device.
int zb_dev_get_id(biz_node_t *pBizNode)
{	
	struct cmd_cont *pcmd;
	
	pcmd = &gCmds_to_coord[CT_READ_ATTR];
	
	pcmd->cmd.cmd_type = 0;
	pcmd->cmd.cmd_cluster_id = 0x0;
	pcmd->cmd.cmd_code = 0x4;	//attr1: Manufacturer id.

	*(zb_uint16 *)pcmd->payload = 0x5;	//atrr2: module ID	
	
	pcmd->payload_len = 2;
	
	send_cmd_to_dev(pBizNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);	
}


#pragma pack(1)

struct bind_req_payload
{
	zb_uint8  dst_mac[8];
	zb_uint8  dst_endp;
	zb_uint16 cluster_id;

	zb_uint8   src_mac[8];
	zb_uint8   src_endp;
};

struct leave_req_payload
{
	zb_uint8  dst_mac[8];
	zb_uint8  option;
};

struct config_rpt_payload
{
	zb_uint8 direction;	// == 0 how it send reports.

	zb_uint16 attr_id;
	zb_uint8  attr_data_type;
	zb_uint16 min_rpt_interval;
	zb_uint16 max_rpt_interval;
	//zb_uint8 reportable_change;
};

struct config_rpt_payload_not_used
{
	zb_uint8 direction;	// == 1
	
	zb_uint16 attr_id;
	zb_uint16 timeout_period; //how it should expect result .
};

struct config_rpt_resp_payload
{
	zb_uint8 status;
	zb_uint8 direction;
	zb_uint16 attr_id;
};

#pragma pack()

int zb_dev_bind_request(biz_node_t *pBizNode, zb_uint8 *dst_mac, zb_uint8 dst_endp,zb_uint16 cluster_id)
{
	struct cmd_cont *pcmd;
	struct bind_req_payload *payload;
	zb_uint8 offset;
	
	pcmd = &gCmds_to_coord[CT_BIND_REQ];
	payload = (struct bind_req_payload *)pcmd->payload;

	memcpy( payload->dst_mac, dst_mac, 8);	
	payload->dst_endp = dst_endp;
	payload->cluster_id = cluster_id;
	
	memset(payload->src_mac, 0, 9);//padding 9 bytes 0. src_mac, src_endp	

	pcmd->payload_len = sizeof(struct bind_req_payload);

	ZB_DBG("-------zb_dev_bind_request()  Bind Request -----\n");
	
	send_cmd_to_dev(pBizNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);	

	return 0;
}

int zb_dev_leave_request(biz_node_t *pBizNode, zb_uint8 *dst_mac, zb_uint8 option)
{	
	struct cmd_cont *pcmd;
	struct leave_req_payload *payload;
	zb_uint8 offset;
	
	pcmd = &gCmds_to_coord[CT_LEAVE_REQ];
	payload = (struct leave_req_payload *)pcmd->payload;

	memcpy( payload->dst_mac, dst_mac, 8);
	payload->option = option;

	pcmd->payload_len = sizeof(struct leave_req_payload);

	send_cmd_to_dev(pBizNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);	
}

int zb_onoff_light(biz_node_t *pBizNode, int on_off)
{
	struct cmd_cont *pcmd;	
	zb_uint8 offset;

	pcmd = &gCmds_to_coord[CT_CMD_ON_OFF];
	pcmd->cmd.cmd_code = on_off;

	pcmd->payload_len = 0;
	
	send_cmd_to_dev(pBizNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);	
}

//configure reporting.
int zb_dev_rpt_configure(biz_node_t *pBizNode )
{
	struct cmd_cont *pcmd;
	struct config_rpt_payload *payload;
	zb_uint8 offset;
	
	pcmd = &gCmds_to_coord[CT_RPT_CONFIG];
	payload = (struct config_rpt_payload *)pcmd->payload;

	payload->direction = 0;
	payload->attr_id = 0; //from DB.  0000 on/off status.
	payload->attr_data_type = ZCL_BOOLEAN_ATTRIBUTE_TYPE; //from DB
	payload->min_rpt_interval = 0;
	payload->max_rpt_interval = 1;	// 1 second. report once.
	//payload->reportable_change = 1; //For bool , it is ommited..	

	if(payload->attr_data_type = ZCL_BOOLEAN_ATTRIBUTE_TYPE)
	{
		pcmd->payload_len = sizeof(struct config_rpt_payload);
	}
	else
	{
		pcmd->payload_len = sizeof(struct config_rpt_payload); //add reportable_change
	}
	
	

	ZB_DBG(" +++  send zb_dev_rpt_configure \n");
	send_cmd_to_dev(pBizNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);	

	return 0;
}


#if 0
int coord_bind_req(zb_uint8 *dst_mac, zb_uint8 dst_endp,zb_uint16 cluster_id)
{
	struct cmd_cont *pcmd;
	struct bind_req_payload *payload;
	zb_uint8 offset;
	
	pcmd = &gCmds_to_coord[CT_BIND_REQ];
	payload = (struct bind_req_payload *)pcmd->payload;

	memcpy( payload->dst_mac, dst_mac, 8);	
	payload->dst_endp = dst_endp;
	payload->cluster_id = cluster_id;
	
	memset(payload->src_mac, 0, 9);//padding 9 bytes 0. src_mac, src_endp	
}
#endif

#if 0
int coord_write_attribute(zb_uint8 on)
{
	
	struct cmd_cont *pcmd;
	
	pcmd = &gCmds_to_coord[CT_WRITE_ATTR];
	
	pcmd->payload[0] = on ? 1 : 0;

	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, 1 );
}
#endif

int coord_write_attribute(zb_uint8 *attr_cont, zb_uint8 attr_cont_len)
{
	
	struct cmd_cont *pcmd;
	
	pcmd = &gCmds_to_coord[CT_WRITE_ATTR];

	if( attr_cont_len > 0)
	{
		memcpy( pcmd->payload, attr_cont, attr_cont_len);
	}
	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, attr_cont_len );
}

int coord_write_cmd(struct cmd_cont *pcmd, zb_uint8 *attr_cont, zb_uint8 attr_cont_len)
{
	
	//struct cmd_cont *pcmd;	
	//pcmd = &gCmds_to_coord[CT_WRITE_ATTR];

	if( attr_cont_len > 0)
	{
		memcpy( pcmd->payload, attr_cont, attr_cont_len);
	}
	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, attr_cont_len );
}


#pragma pack(1)
struct dev_announce_payload
{
	zb_uint16 short_id;
	zb_uint8  mac[8];
	zb_uint8  cap;
};

struct dev_attr_payload
{
	zb_uint16 cmd_code;
	zb_uint8  status;
	zb_uint8  dataType;	
};
#pragma pack()

zb_uint8 tmp_cont[256];
void zb_attr_get_ids(struct zb_ser_frame_hdr *pFrame)
{
	struct dev_attr_payload *pattr;
	int size;
	int i =0;
	zb_uint8 *p;

	pattr = (struct dev_attr_payload *)((zb_uint8 *)pFrame 	+ ZB_HDR_LEN -2); //start at cmd_code

	while( (zb_uint8 *)pattr < (zb_uint8 *)pFrame + pFrame->len)
	{
		//ZB_DBG("pattr:%p  end:%p\n", (zb_uint8 *)pattr, (zb_uint8 *)pFrame + pFrame->len + 2);
		
		if(0 == pattr->status) //success.
		{
			p = (zb_uint8 *)pattr + sizeof(struct dev_attr_payload);
			
			size = zb_attr_get_simple_val( pattr->dataType, p, tmp_cont );
			if(0 == size )
			{
				ZB_DBG("> Failed to extract data\n", size);
				return;
			}
			
			p += size;			
			
			if( pattr->cmd_code == 0x04)
			{
				ZB_DBG("> Product ID (size=%d):", size-1);
				
				for(i=0; i<size; i++)
				{
					ZB_DBG("%c", tmp_cont[i] );
				}
				ZB_DBG("\n");
			}
			else if( pattr->cmd_code == 0x05)
			{
				ZB_DBG("> Module ID (size=%d):",size-1);
				
				for(i=0; i<size; i++)
				{
					ZB_DBG("%c", tmp_cont[i] );
				}
				ZB_DBG("\n");
			}

			pattr =  (struct dev_attr_payload *)(p);
		}
		else
		{
			ZB_DBG("> Read Attr response return fail status.\n");
			pattr = (struct dev_attr_payload *)((zb_uint8 *)pattr + 3);	//cmd_code + status, not pendding data.
		}
	}
}

//return 0 when all attr reporting is supportd.
//
int zb_rpt_resp_extract(struct zb_ser_frame_hdr *pFrame)
{
	struct config_rpt_resp_payload *resp;

	resp = (struct config_rpt_resp_payload *)( (zb_uint8 *)pFrame + ZB_HDR_LEN );

	do
	{
		if( resp->status == 0)	//success.
		{
			ZB_DBG("!!!! Configure Reporting Support attr:0x%x !!!!\n", resp->attr_id);		
		}
		else
		{
			ZB_DBG("!!!! Configure Reporting Not support attr:0x%x !!!!\n", resp->attr_id);
			return -1;
		}

		resp = resp + 1;  //next response.
	}while( (zb_uint8 *)resp < (zb_uint8 *)pFrame + pFrame->len);

	return 0;	
}

int coord_msg_handle(struct zb_ser_frame_hdr *pFrame, struct serial_session_node *pnode)
{
	int ret = 0;
	biz_node_t *pBizNode;
	zb_uint8 *payload = (zb_uint8 *)pFrame + ZB_HDR_LEN;
	struct serial_session_node *psNew;
	ser_addr_t new_dev_addr;
	int retval;
		
	ZB_DBG(">> coord_msg_handle in. <<<\n");

	pBizNode = biz_find_node(0,  pFrame->src_addr.short_addr, pFrame->src_addr.endp_addr );

	if( pBizNode == NULL)
	{
		//device announce get.
		if( (pFrame->cmd.cmd_type == 0x40) && (pFrame->cmd.cmd_cluster_id == 0x13) && (pFrame->cmd.cmd_code == 0) )
		{
			struct dev_announce_payload *p = (struct dev_announce_payload *)((zb_uint8 *)pFrame + ZB_HDR_LEN);
			int cnt, i;
			zb_uint8 endp_addr;
			biz_node_t * pbz_node;

			zb_dev_add( p->short_id, p->mac, p->cap);

			//check the DB to get how many endp this dev has.
			cnt = zb_dev_get_endp_cnt(p->short_id);
			if( cnt < 0)
			{
				ZB_ERROR("!!! This device didn't have any endpoint, should be DB configuration error ?");	
			}

			ZB_DBG("!!! Device announce received: dev_short_id:0x%x \n", p->short_id);
			ZB_DBG("!!! MAC: %x %x %x %x %x %x %x %x\n", p->mac[0],p->mac[1],p->mac[2],p->mac[3],
														p->mac[4],p->mac[5],p->mac[6],p->mac[7]);

			for(i=0; i<cnt; i++)
			{	
				endp_addr = zb_dev_get_endp(p->short_id, i);

				ZB_DBG("    !!! dev_endpoint:0x%x \n", endp_addr);
				
				pbz_node = (biz_node_t *)biz_find_or_add_node(0, p->short_id, endp_addr);
				if( pbz_node == NULL)
				{
					ZB_ERROR("    !!! Failed to allocate device node in [Device Announce].\n");
				}
				memcpy( pbz_node->dev_mac, p->mac, 8);
				
				//create a new session node with the correct address.				
				new_dev_addr.short_addr = p->short_id;
				new_dev_addr.endp_addr = endp_addr;
				psNew = find_or_add_serial_session( &host_addr, &new_dev_addr);
				pbz_node->pSerialSessNode = psNew;
				
				pbz_node->zb_biz_status = ZB_BIZ_GOT_DEVICE_ANNOUNCE;
				add_node_to_search_list( pbz_node );
			}
		}
		else
		{
			ZB_DBG("-------BIZ node not found. not device announce msg---\n");
		}
		return 0;
	}

	ZB_DBG("\n\t pBizNode(%p) status:%d \n\n", pBizNode, pBizNode->zb_biz_status);

	if( pBizNode->zb_biz_status >= ZB_BIZ_SEARCH_COMPLETE)
	{
		ZB_DBG("-----zb_biz_status = %d, normal process.(pBizNode=%p)------\n", pBizNode->zb_biz_status, pBizNode);
	}
	else
	{	
		switch( pBizNode->zb_biz_status )
		{			
			case ZB_BIZ_READING_ID:
			{				
				//This is read attribute response. and cmd is reading ID.
				if( (pFrame->cmd.cmd_type == 0x01) && (pFrame->cmd.cmd_cluster_id == 0) && (pFrame->cmd.cmd_code == 0x0004) )
				{
					zb_attr_get_ids(pFrame);
					
					//if this ID is our cared. process it, else ignore it.
					if(1)
					{
						pBizNode->zb_biz_status = ZB_BIZ_GOT_ID;	
					}
					else
					{
						pBizNode->zb_biz_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
					}			
					add_node_to_search_list( pBizNode );
				}
				else
				{
					ZB_DBG("!!! Not Read Id cmd response.!!!\n");
				}
				return 0;
			}
			case ZB_BIZ_BIND_SENT:
			{
				//ZB_DBG("---status: ZB_BIZ_BIND_SENT (pBizNode:%p)---\n", pBizNode);
				
				//Bind Response
				if( (pFrame->cmd.cmd_type == 0x40) && (pFrame->cmd.cmd_cluster_id == 0x8021) && (pFrame->cmd.cmd_code == 0) )
				{					
					if( *payload == 0 )	//success.
					{
						pBizNode->zb_biz_status = ZB_BIZ_BIND_RESP;	//bind success.
						ZB_DBG("---Bind Response  success---\n");
					}
					else
					{
						pBizNode->zb_biz_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;  //bind fail.
						ZB_DBG("---Bind Response  fail---\n");
					}
					add_node_to_search_list( pBizNode );
				}
				else
				{
					ZB_DBG("!!! Not bind cmd response.!!!\n");
				}

				return ret;
			}
			case ZB_BIZ_CONFIG_REPORTING_SENT:
			{				
				if((pFrame->cmd.cmd_type == 0x07))	//configure response
				{					
					retval = zb_rpt_resp_extract(pFrame);
					if( 0 == retval)
					{
						//all attr is supported.
						pBizNode->zb_biz_status = ZB_BIZ_CONFIG_REPORTING_RESP;
						add_node_to_search_list( pBizNode );

						ZB_DBG("------ GOT Configure Response : all success ------\n");
					}
					else
					{
						//Treat it as add device FAIL. 
						//debug only...
						pBizNode->zb_biz_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
						add_node_to_search_list( pBizNode );

						ZB_DBG("------ GOT Configure Response : part fail ------\n");
					}					
				}
				else if((pFrame->cmd.cmd_type == 0x0b))	//default response, means cmd fail, should not occur in release field.
				{
					ZB_DBG("------ GOT Default Response means configure fail ------\n");
					pBizNode->zb_biz_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
					add_node_to_search_list( pBizNode );
				}
				else
				{
					ZB_DBG("------ UnExpected response ------\n");
					pBizNode->zb_biz_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
					add_node_to_search_list( pBizNode );
				}				
				
				return ret;
			}
		}
	}

	return ret;
}

