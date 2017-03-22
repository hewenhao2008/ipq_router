#include "zb_cmds_join.h"
#include "biz_msg_process.h"

#include "dev_mgmt.h"
#include "dev_search_thread.h"
#include "zb_data_type.h"
#include "zb_cmds.h"

#pragma pack(1)
struct bind_req_payload
{
	zb_uint8  dst_mac[8];
	zb_uint8  dst_endp;
	zb_uint16 cluster_id;

	zb_uint8   src_mac[8];
	zb_uint8   src_endp;
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
	zb_uint16 attr_id;
	zb_uint8 status;
	zb_uint8 direction;	
};


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

/*
* command sent by host in join phase.
*/

// search device. allow them to join network.
int host_send_cmd_permit_join(biz_node_t *pBizNode)
{
	struct cmd_cont *pcmd;
	pcmd = coord_get_global_cmd(CT_SEARCH_DEV);

	pBizNode->pSerialSessNode = host_get_session_with_coord();

	biz_set_node_doing_search(pBizNode);

	ZB_DBG("--------coord_search_dev(): Search Device Permit Join --------\n");

	//send_cmd_to_coord( (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len );
	zb_protocol_send( host_get_session_with_coord(),
			(zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len );
	
	return 0;
}

//read manufactory id and module id in a single command.
int host_send_get_ids(struct serial_session_node *pNode)
{	
	struct cmd_cont *pcmd;
	pcmd = coord_get_global_cmd(CT_READ_ATTR);
	//pcmd = &gCmds_to_coord[CT_READ_ATTR];
	
	pcmd->cmd.cmd_type = 0;
	pcmd->cmd.cmd_cluster_id = 0x0;
	pcmd->cmd.cmd_code = 0x4;	//attr1: Manufacturer id.

	*(zb_uint16 *)pcmd->payload = 0x5;	//atrr2: module ID	
	
	pcmd->payload_len = 2;
	pNode->seq = get_new_seq_no(1);
	
	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);
}

//bind request. the attribute should be provided by user.
int host_send_bind_request(struct serial_session_node *pNode, zb_uint8 *dst_mac, zb_uint8 dst_endp,zb_uint16 cluster_id)
{
	struct cmd_cont *pcmd;
	struct bind_req_payload *payload;
	zb_uint8 offset;
	
	//pcmd = &gCmds_to_coord[CT_BIND_REQ];
	pcmd = coord_get_global_cmd(CT_BIND_REQ);	
	
	payload = (struct bind_req_payload *)pcmd->payload;

	memcpy( payload->dst_mac, dst_mac, 8);	
	payload->dst_endp = dst_endp;
	payload->cluster_id = cluster_id;
	
	memset(payload->src_mac, 0, 9);//padding 9 bytes 0. src_mac, src_endp	

	pcmd->payload_len = sizeof(struct bind_req_payload);

	pNode->seq = get_new_seq_no(1);

	ZB_DBG("-------zb_dev_bind_request()  Bind Request (seq:%x)-----\n", pNode->seq);
	
	//send_cmd_to_dev(pBizNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);	
	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);
	
	return 0;
}

//configure reporting.
int host_send_rpt_configure(struct serial_session_node *pNode )
{
	struct cmd_cont *pcmd;
	struct config_rpt_payload *payload;
	zb_uint8 offset;
	
	//pcmd = &gCmds_to_coord[CT_RPT_CONFIG];
	pcmd = coord_get_global_cmd(CT_RPT_CONFIG);	
	
	payload = (struct config_rpt_payload *)pcmd->payload;

	payload->direction = 0;
	payload->attr_id = 0; //from DB.  0000 on/off status.
	payload->attr_data_type = ZCL_BOOLEAN_ATTRIBUTE_TYPE; //from DB
	payload->min_rpt_interval = 5;	// can not be 0.
	payload->max_rpt_interval = 60;	// 60 second. report once.
	//payload->reportable_change = 1; //For bool , it is ommited..	

	if(payload->attr_data_type = ZCL_BOOLEAN_ATTRIBUTE_TYPE)
	{
		pcmd->payload_len = sizeof(struct config_rpt_payload);
	}
	else
	{
		pcmd->payload_len = sizeof(struct config_rpt_payload); //add reportable_change
	}
	
	pNode->seq = get_new_seq_no(1);	

	ZB_DBG(" +++  send zb_dev_rpt_configure \n");

	zb_protocol_send(pNode, (zb_uint8 *)&pcmd->cmd, pcmd->payload, pcmd->payload_len);
	
	return 0;
}

/**************************************************************/


/*
* Handle msg from coordinate & zigbee evice.
*/

//Device announce msg. Msg from device.
int coord_msg_handle_device_announce(struct zb_ser_frame_hdr *pFrame)
{
	biz_node_t *pBizNode;
	struct serial_session_node * pnode;
	struct serial_session_node * pNew;
	ser_addr_t new_dev_addr;

	ZB_DBG("----------Handle Device announce------\n");
	
	pBizNode = biz_get_node_doing_search();
	if( pBizNode == NULL)
	{
		ZB_DBG(">> We are not searching or adding device, ignore device announce.\n");
		return 0;
	}
	else
	{
		struct dev_announce_payload *p = (struct dev_announce_payload *)((zb_uint8 *)pFrame + ZB_HDR_LEN);
		int cnt, i;
		zb_uint8 endp_addr;
		
		if( pFrame->src_addr.endp_addr != 0)
		{
			ZB_DBG(">>!!! Device announce frame should use endpoint 0 (ZDO). !!! \n");
		}

		//find session Node. switch dst and src.				

		//switch dst addr and src addr.
		pnode = find_dev_from_join_list( &pFrame->src_addr );
		if( pnode != NULL)
		{
			ZB_DBG(">> Device announce again, Ignore it.\n");
			return 0;
		}
		else
		{
			//add device. ZDO, endp==0
			pnode = add_dev_to_join_list( host_get_hostaddr(), &pFrame->src_addr); //switch addr.
			if( pnode == NULL)
			{
				return -1;
			}
		}

		//Now pnode is ZDO device.
		pnode->rx_seq = pFrame->seq;	

		//list_add( &pnode->chain_for_adding, &pBizNode->endp_chain);
		pnode->pBizNodeDoingAdding = pBizNode;
		pnode->zb_join_status = ZB_BIZ_GOT_DEVICE_ANNOUNCE;

		//pBizNode->zb_biz_status[0] = ZB_BIZ_GOT_DEVICE_ANNOUNCE;	

		//debug, can output mac addr of this device.
		memcpy( pnode->mac, p->mac, 8);
		
		//2. check the DB to get how many endp this dev has. Not by short_id, by device name.
		//cnt = zb_dev_get_endp_cnt(p->short_id);
		cnt = pBizNode->endpoint_cnt;
		if( cnt < 0)
		{
			ZB_ERROR("!!! This device didn't have any endpoint, should be DB configuration error ?");	

			//Delete device from List.
			//list_del( &pnode->chain_for_adding);
			pnode->pBizNodeDoingAdding = NULL;				
			return 0;
		}

		//add biz node for each endpoint.
		for(i=0; i<cnt; i++)
		{	
			//endp_addr = zb_dev_get_endp_addr(p->short_id, i);
			endp_addr = pBizNode->endpoint_addrs[i];

			ZB_DBG("    !!! dev_endpoint:0x%x \n", endp_addr);				
			
			//create a new session node with the correct address.				
			new_dev_addr.short_addr = p->short_id;
			new_dev_addr.endp_addr = endp_addr;
			pNew = add_endp_to_join_list( host_get_hostaddr(), &new_dev_addr);

			memcpy( pNew->mac, p->mac, 8);	//store MAC addr.

			//list_add( &pNew->chain_for_adding, &pBizNode->endp_chain );
			pNew->pBizNodeDoingAdding = pBizNode;
							
			pNew->zb_join_status = ZB_BIZ_GOT_DEVICE_ANNOUNCE; //not need?	

			add_node_to_search_list_new( pNew );
		}			
	}

	return 0;
}

//Bind response. Msg from device.
int coord_msg_handle_bind_resp(struct zb_ser_frame_hdr *pFrame)
{
	struct serial_session_node *pNode;
	zb_uint8 *payload = (zb_uint8 *)pFrame + ZB_HDR_LEN;

	ZB_DBG("----------Handle Bind Resp ------\n");
	
	//Find endp by seq and short_addr.
	pNode = find_endp_from_join_list_byseq( pFrame->src_addr.short_addr, pFrame->seq);
	if( pNode == NULL)
	{
		ZB_DBG("---Bind Response : didn't find origial endp.[short_addr:%x seq:%x]\n", 
				pFrame->src_addr.short_addr,
				pFrame->seq);
		return -1;
	}

	if( pNode->zb_join_status != ZB_BIZ_BIND_SENT)
	{
		ZB_DBG("!!! Bind Response : endp join status wrong. not at ZB_BIZ_BIND_SENT. status=%x\n", 
				pNode->zb_join_status);		
	}
		
	//Bind Response						
	if( *payload == 0 )	//success.
	{
		pNode->zb_join_status = ZB_BIZ_BIND_RESP;	//bind success.
		ZB_DBG("---Bind Response  success---\n");
	}
	else
	{
		pNode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;  //bind fail.
		ZB_DBG("---Bind Response  fail---\n");
	}
	
	add_node_to_search_list_new( pNode  );	
}

//Msg from endpoint.
void coord_msg_decode_getids_resp(struct zb_ser_frame_hdr *pFrame, char *manufacture, char *moduleId)
{
	struct dev_attr_payload *pattr;
	static zb_uint8 tmp_attr_id_cont[256];
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
			
			size = zb_attr_get_simple_val( pattr->dataType, p, tmp_attr_id_cont );
			if(0 == size )
			{
				ZB_DBG("> Failed to extract data\n", size);
				return;
			}
			
			p += size;			
			
			if( pattr->cmd_code == 0x04)
			{
				ZB_DBG("> Product ID (size=%d):", size-1);
				
				for(i=0; (i<size) && (i<BIZ_MANUFACTURE_NAME_LEN); i++)
				{
					ZB_DBG("%c", tmp_attr_id_cont[i] );
					manufacture[i] = tmp_attr_id_cont[i];
				}
				ZB_DBG("\n");				
			}
			else if( pattr->cmd_code == 0x05)
			{
				ZB_DBG("> Module ID (size=%d):",size-1);
				
				for(i=0; (i<size) && (i<BIZ_MODULE_ID_LEN); i++)
				{
					ZB_DBG("%c", tmp_attr_id_cont[i] );
					moduleId[i] = tmp_attr_id_cont[i];
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



//Msg from endpoint.
//return 0 when all attr reporting is supportd. 
int coord_msg_decode_rpt_resp(struct zb_ser_frame_hdr *pFrame)
{
	struct config_rpt_resp_payload *resp;

	//payload overwrite cmd code.
	resp = (struct config_rpt_resp_payload *)( (zb_uint8 *)pFrame + ZB_HDR_LEN - 2); 

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

//Msg from endpoint.
int coord_msg_handle_other_join_msg(struct zb_ser_frame_hdr *pFrame)
{
	struct serial_session_node * pnode;
	biz_node_t *pBizNode;
	
	char manufactureName[BIZ_MANUFACTURE_NAME_LEN];
	char ModuleId[BIZ_MODULE_ID_LEN];
	int retval;

	ZB_DBG("------ Handle Normal Join Msg ----\n");
	
	pnode = find_endp_from_join_list( &pFrame->src_addr, pFrame->seq);
	if( pnode == NULL)
	{
		ZB_ERROR("!!! Join phase, Msg from unknown device.[short_id:%x endp:%x]\n", 
					pFrame->src_addr.short_addr, pFrame->src_addr.endp_addr);
		return -1;
	}

	if( !pnode->pBizNodeDoingAdding)
	{
		ZB_ERROR("!!! Join phase, Node in join phase.[short_id:%x endp:%x]\n", 
					pFrame->src_addr.short_addr, pFrame->src_addr.endp_addr);
		return -1;
	}
	pBizNode = pnode->pBizNodeDoingAdding;

	ZB_DBG("-----pnode->zb_join_status=%x\n", pnode->zb_join_status);
	

	switch(pnode->zb_join_status)
	{		
		case ZB_BIZ_READING_ID:
		{				
			//This is read attribute response. and cmd is reading ID.
			if( (pFrame->cmd.cmd_type == 0x01) && (pFrame->cmd.cmd_cluster_id == 0) && (pFrame->cmd.cmd_code == 0x0004) )
			{				
				memset(manufactureName, 0, BIZ_MANUFACTURE_NAME_LEN);
				memset(ModuleId, 0, BIZ_MODULE_ID_LEN);
				
				coord_msg_decode_getids_resp(pFrame, manufactureName, ModuleId);
				
				//if this ID is our cared. process it, else ignore it.
				if( (!strncmp(manufactureName, pBizNode->manufactureName, BIZ_MANUFACTURE_NAME_LEN)) &&
					(!strncmp(ModuleId, pBizNode->moduleId, BIZ_MODULE_ID_LEN)))
				{
					pnode->zb_join_status = ZB_BIZ_GOT_ID;	
					ZB_DBG(">> Manufacture and module ID match.\n");
				}
				else
				{
					pnode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
					ZB_DBG(">> Not adding this device... Manufacture and module ID NOT match\n");
				}			
				add_node_to_search_list_new( pnode );
			}
			else
			{
				ZB_DBG("!!! Not Read Id cmd response.!!!\n");
			}
			return 0;
		}
		case ZB_BIZ_CONFIG_REPORTING_SENT:
		{				
			if((pFrame->cmd.cmd_type == 0x07))	//configure response
			{					
				retval = coord_msg_decode_rpt_resp(pFrame);
				if( 0 == retval)
				{
					//all attr is supported.
					pnode->zb_join_status = ZB_BIZ_CONFIG_REPORTING_RESP;
					pnode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE;
					
					dev_complete_join( pnode->dst_addr.short_addr );

					ZB_DBG("------ GOT Configure Response : all success ------\n");
				}
				else
				{
					//Treat it as add device FAIL. 
					#if 0
						pnode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;					
						host_send_dev_leave_request( pnode, pnode->mac, 0);
						dev_delete_join_phase( pnode->dst_addr.short_addr);
					#else
						ZB_DBG("---Debug only, should remove when release.------\n");
						pnode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE;					
						dev_complete_join( pnode->dst_addr.short_addr );
					#endif
					ZB_DBG("------ GOT Configure Response : part fail ------\n");
				}					
			}
			else if((pFrame->cmd.cmd_type == 0x0b))	//default response, means cmd fail, should not occur in release field.
			{
				ZB_DBG("------ GOT Default Response means configure fail ------\n");
				pnode->zb_join_status  = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
				#if 0				
					host_send_dev_leave_request( pnode, pnode->mac, 0); //BE Careful. TX may be mixed with tx thread.
					dev_delete_join_phase( pnode->dst_addr.short_addr );
				#else
					pnode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE;
					dev_complete_join( pnode->dst_addr.short_addr );
				#endif
			}
			else
			{
				ZB_DBG("------ UnExpected response ------\n");
				//pnode->zb_join_status = ZB_BIZ_SEARCH_COMPLETE_DEL_IT;
				//add_node_to_search_list_new( pnode );
			}				
			
			return 0;
		}
		default:
		{
			ZB_DBG(">> Bad status. status=%x\n", pnode->zb_join_status);
			return 0;
		}
	}

	return 0;	
}

//Entry point of Handling all msg from coordinate and device .
int coord_msg_handle_dev_join(struct zb_ser_frame_hdr *pFrame)
{	
	//device announce. From ZDO
	if( (pFrame->cmd.cmd_type == 0x40) && (pFrame->cmd.cmd_cluster_id == 0x13) && (pFrame->cmd.cmd_code == 0) )
	{
		return coord_msg_handle_device_announce(pFrame);		
	}

	//If bind reponse. From ZDO.
	if( (pFrame->cmd.cmd_type == 0x40) && (pFrame->cmd.cmd_cluster_id == 0x8021) && (pFrame->cmd.cmd_code == 0) )
	{
		return coord_msg_handle_bind_resp(pFrame);		
	}

	//Resp to endp.
	return	coord_msg_handle_other_join_msg(pFrame);	
}



