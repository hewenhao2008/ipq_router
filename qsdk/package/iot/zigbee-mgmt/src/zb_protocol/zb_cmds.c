#include "common_def.h"
#include "zb_serial_protocol.h"
#include "biz_msg_process.h"
#include "zb_data_type.h"
#include "zb_cmds.h"

int coord_msg_handle(struct zb_ser_frame_hdr *pFrame);


#define HOST_ADDR_ENDP_ADDR 0x1
#define COORD_ADDR_ENDP_ADDR 0xF1

ser_addr_t host_addr = { .short_addr=0, 
					.endp_addr= HOST_ADDR_ENDP_ADDR };
					
ser_addr_t coord_addr = { .short_addr=0, 
						 .endp_addr= COORD_ADDR_ENDP_ADDR };;

//the session node to communicate with coordinate.
struct serial_session_node * gSessWithCoord;

struct serial_session_node * host_get_session_with_coord()
{
	return gSessWithCoord;
}

ser_addr_t * host_get_hostaddr(void)
{
	return &host_addr;
}

ser_addr_t * host_get_coordaddr(void)
{
	return &coord_addr;
}


struct cmd_cont gCmds_to_coord[] = {

	/**** C_SEARCH_DEV =0 *****/
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

	/**** CF_JOIN_INFO_RPT 1****/
	{
		.index = CF_JOIN_INFO_RPT,
		.cmd = {
					.cmd_type=0x0E, 
			 		.cmd_cluster_id=0xFE00, 
			 		.cmd_code=0x01
				},
	  	.payload = {0}	// dev info.
	},

	/**** CT_DEL_DEV 2****/ 
	{
		.index = CT_DEL_DEV,
	},

	/******* CT_WRITE_ATTR 3*************/
	{
		.index = CT_WRITE_ATTR,
		.cmd = {
					.cmd_type=CMD_WRITE_ATTR,  //02
			 		.cmd_cluster_id=0x0006,	//ON/OFF 
			 		.cmd_code=0
				},
	  	.payload = {0}	// dev info.
	},

	/***** CT_READ_ATTR 4**************/
	{
		.index = CT_READ_ATTR,
		.cmd = {
					.cmd_type= 0,  //00
			 		.cmd_cluster_id=0,	//decide by attr.
			 		.cmd_code=0x04
				},
	  	.payload = {0}	// dev info.
	},

	/********CT_BIND_REQ 5***************/
	{
		.index = CT_BIND_REQ,
		.cmd = {
					.cmd_type= 0x40,  
			 		.cmd_cluster_id=0x21,	
			 		.cmd_code=0x0
				},
	  	.payload = {0}	// dst_mac+dst_endp_clusterid
	},

	/**********CT_RPT_CONFIG 6********************/
	{		
		.index = CT_RPT_CONFIG,
		.cmd = {
					.cmd_type= 0x06,  
			 		.cmd_cluster_id=0x06, //depends on attr	
			 		.cmd_code=0xC0DE	  //Not used.
				},
	  	.payload = {0}	
	},

	/**** CT_LEAVE_REQ 7**************/
	{
		.index = CT_LEAVE_REQ,
		.cmd = {
					.cmd_type= 0x40,  
			 		.cmd_cluster_id=0x0034,	
			 		.cmd_code=0x1
				},
	  	.payload = {0}	
	},

	/**** CT_CMD_ON_OFF 8**************/
	{
		.index = CT_CMD_ON_OFF,
		.cmd = {
					.cmd_type= 0x41,  
			 		.cmd_cluster_id=0x0006,	
			 		.cmd_code=0x2
				},
	  	.payload = {0}	
	},

	/****** CT_LIGHT_MOVE_TO_COLOR 9*********/
	{
		.index = CT_LIGHT_MOVE_TO_COLOR,
		.cmd = {
					.cmd_type = 0x41,
					.cmd_cluster_id = 0x0300,
					.cmd_code = 0x7
			   },
		.payload = {1, 2}	//colorX colorY Transition
	},

	/**** CT_LIGHT_MOVE_TO_COLOR_TEMPERATURE ****/
	{
		.index = CT_LIGHT_MOVE_TO_COLOR_TEMPERATURE,
		.cmd = {
					.cmd_type = 0x41,
					.cmd_cluster_id = 0x0300,
					.cmd_code = 0xA
			   },
		.payload = {1, 2}	//Temp Transition
	},

	/***** CT_LIGHT_MOVE_TO_LEVEL ****/
	{
		.index = CT_LIGHT_MOVE_TO_LEVEL,
		.cmd = {
					.cmd_type = 0x41,
					.cmd_cluster_id = 0x0008,
					.cmd_code = 0
			   },
		.payload = {1, 2}	//Level Transition
	},
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

struct cmd_cont * coord_get_global_cmd(zb_uint8 index)
{
	return &gCmds_to_coord[index];
}



/////////////////////////////////////////////////////////////////////////////
#pragma pack(1)
struct leave_req_payload
{
	zb_uint8  dst_mac[8];
	zb_uint8  option;
};
#pragma pack()


int coord_get_attr_single(biz_node_t *pBizNode, zb_uint16 attr_custer_id, zb_uint16 attr_code)
{
	struct cmd_cont *pcmd;

	pcmd = &gCmds_to_coord[CT_READ_ATTR];
	pcmd->cmd.cmd_cluster_id = attr_custer_id;
	pcmd->cmd.cmd_code = attr_code;

	pBizNode->pSerialSessNode = gSessWithCoord;
	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len );
}

int host_send_dev_leave_request(struct serial_session_node *pNode, zb_uint8 *dst_mac, zb_uint8 option)
{	
	struct cmd_cont *pcmd;
	struct leave_req_payload *payload;
	zb_uint8 offset;
	
	pcmd = &gCmds_to_coord[CT_LEAVE_REQ];
	payload = (struct leave_req_payload *)pcmd->payload;

	memcpy( payload->dst_mac, dst_mac, 8);
	payload->option = option;

	pcmd->payload_len = sizeof(struct leave_req_payload);

	pNode->seq = get_new_seq_no(0);	

	ZB_DBG(" +++  send leave request +++ \n");

	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);
}


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
	if( attr_cont_len > 0)
	{
		memcpy( pcmd->payload, attr_cont, attr_cont_len);
	}
	send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, attr_cont_len );
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

#pragma pack(1)
struct dev_attr_rpt
{
	zb_uint16 cmd_code;
	zb_uint8  status;
	zb_uint8  dataType;	
};
#pragma pack()


struct zb_attr
{
	zb_uint16 cluster_id;
	zb_uint16 attr_id;
	zb_uint16 attr_dtype;		//data type.
	zb_uint16 attr_dmax_len;	//data max length.
	char     *attr_name;			//be used for json.
};

struct zb_attr attr_list[] = {
	//Light, CurrentX CurrentY
	{.cluster_id=0x0300, .attr_id=0x0003, .attr_dtype=ZCL_INT16U_ATTRIBUTE_TYPE, .attr_dmax_len=2, .attr_name="CurrentX" },
	{.cluster_id=0x0300, .attr_id=0x0004, .attr_dtype=ZCL_INT16U_ATTRIBUTE_TYPE, .attr_dmax_len=2, .attr_name="CurrentY"  },
	
	//ColorTemperature
	{.cluster_id=0x0300, .attr_id=0x0007, .attr_dtype=ZCL_INT16U_ATTRIBUTE_TYPE, .attr_dmax_len=2, .attr_name="ColorTemperature" },

};

//return the length of the attribute value.
// -1 means error.
int coord_msg_attr_payload_extract(struct dev_attr_rpt *pattr, zb_uint16 cluster_id)
{
	struct zb_attr *plist;
	int found = 0;
	zb_uint8 size;
	zb_uint8 buf[256];
	zb_uint8 *p_cont;
	int i;
	
	if( pattr->status != 0)
	{
		return -1;
	}

	for(i=0; i<sizeof(attr_list)/sizeof(struct zb_attr); i++)
	{
		plist = &attr_list[i];
		if( (cluster_id == plist->cluster_id) && (pattr->cmd_code == plist->attr_id))
		{
			found= 1;
			break;
		}
	}

	if( !found)
		return -1;

	p_cont = ((zb_uint8 *)pattr + sizeof(struct dev_attr_rpt));

	memset(buf, 0, 256);
	
	size = zb_attr_get_simple_val( pattr->dataType, p_cont, buf );	
	
	ZB_DBG(">Attribute Name :%s ", plist->attr_name);
	ZB_DBG(">Attribute value:");
	
	switch(pattr->dataType)
	{
		case ZCL_INT16U_ATTRIBUTE_TYPE:
				ZB_DBG(">Attribute value:%u\n", (zb_uint16*)buf);
		default:
			break;
	}

	return size;	
}


int coord_msg_decode_attr_rpt(struct zb_ser_frame_hdr *pFrame)
{
	struct dev_attr_rpt *pattr;
	static zb_uint8 tmp_attr_id_cont[256];
	int size;
	int i =0;
	zb_uint8 *p;

	pattr = (struct dev_attr_rpt *)((zb_uint8 *)pFrame 	+ ZB_HDR_LEN -2); //start at cmd_code

	while( (zb_uint8 *)pattr < (zb_uint8 *)pFrame + pFrame->len)
	{
		if( pattr->status == 0)
		{
			//status ok.
			
			size = coord_msg_attr_payload_extract( pattr, pFrame->cmd.cmd_cluster_id );
			if(size == -1)
			{
				//Not supported attribute.
				p = (zb_uint8 *)pattr + sizeof(struct dev_attr_rpt);
				size = zb_attr_get_simple_val(pattr->dataType, p, tmp_attr_id_cont );
			}
			
			p = (zb_uint8 *)pattr + sizeof(struct dev_attr_rpt) + size;	//next attr.		
			pattr =  (struct dev_attr_rpt *)(p);
		}		
		else
		{
			pattr = (struct dev_attr_rpt *)((zb_uint8 *)pattr + 3); //cmd_code + status, not pendding data.
		}
	}
}



//int coord_msg_handle(struct zb_ser_frame_hdr *pFrame, struct serial_session_node *pnode)
int coord_msg_handle(struct zb_ser_frame_hdr *pFrame)
{
	int ret = 0;
	biz_node_t *pBizNode;
	zb_uint8 *payload = (zb_uint8 *)pFrame + ZB_HDR_LEN;
	struct serial_session_node *psNew;
	ser_addr_t new_dev_addr;
	int retval;
		
	ZB_DBG(">> coord_msg_handle in. <<<\n");

	if((pFrame->cmd.cmd_type == 0x0A))	//device reporting attr.
	{
		ZB_DBG("---device reporting attr now ---.\n");
		
		coord_msg_decode_attr_rpt(pFrame);
		
		//send msg to server.
		return 0;
	}

	//Device announce. seq generated by MCU, recognize it separately.
	if( (pFrame->cmd.cmd_type == 0x40) && (pFrame->cmd.cmd_cluster_id == 0x13) && (pFrame->cmd.cmd_code == 0) )
	{
		return coord_msg_handle_dev_join(pFrame);
	}
	else if( pFrame->seq < SEQ_DEV_JOIN_PHASE_MAX )	//response seq in Join Phase.
	{
		return coord_msg_handle_dev_join(pFrame);
	}	
	else	//request seq in working phase. normal frame handle.
	{	
		struct serial_session_node * pnode;

		//switch dst addr and src addr.
		pnode = find_serial_session(&pFrame->dst_addr, &pFrame->src_addr);
		if( pnode == NULL)
		{
			ZB_ERROR(">> Msg from unknown device. ignore it [short_dev:%x endp:%x].\n",
						pFrame->src_addr.short_addr, pFrame->src_addr.endp_addr);
			return -1;
		}

		if( pnode->zb_join_status < ZB_BIZ_SEARCH_COMPLETE)
		{
			ZB_ERROR("!!! endp status should not stay in JOIN state. !!!");
		}

		pnode->rx_seq = pFrame->seq;	
		
		return 0;
	}
}

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


