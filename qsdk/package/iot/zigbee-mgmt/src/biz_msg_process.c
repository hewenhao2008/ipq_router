#include "biz_msg_process.h"
#include "biz_msg_parser.h"
#include "kernel_list.h"

int biz_msg_handle(char *msg, int msgLen)
{
	int ret;
	biz_msg_t *pbiz_msg;

	pbiz_msg = malloc(sizeof(biz_msg_t));
	if( NULL == pbiz_msg)
	{
		return -1;
	}

	ret = parse_app_msg(msg, pbiz_msg);
	if( ret != 0)
	{
		return -1;
	}

	switch(pbiz_msg->hdr.cmd_type)
	{
		case CMD_WRITE:
		{
			// biz_msg_queue_ongoing.
			
			break;
		}
		case CMD_READ:
		{
			
			break;
		}
		case CMD_ADD_DEV:
		{
			
		}
		default:
		{
			break;
		}		
	}
	
}


zb_uint8 payload[256];

int biz_write_attr(biz_msg_t *pbiz_msg)
{
	int i;
	zb_uint8 offset = 0;
	
	for(i=0; (i < pbiz_msg->attr_cnt)&&(i < BIZ_MSG_MAX_ATTR_NUM); i++)
	{
		//get code of attribute. ON/OFF -> 0
		offset += snprintf( payload + offset, 255, "%s", pbiz_msg->u.attrs[i].attr);
		offset += snprintf( payload + offset, 255, "%s", pbiz_msg->u.attrs[i].val);
	}
	
	coord_write_attribute(payload, offset);
}

// 1 second get response.
#define CMD_SEARCH_DEV_TIMEOUT_NS	1000*1000*1000

// could_id = 0, shortid=0, endp_id = 0xF1 (COORD_ADDR_ENDP_ADDR);
int biz_search_dev(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id)
{
	biz_node_t *pbiz_node;
	int rc;
	int ret;
	struct timespec absTime;

	ZB_DBG(">> biz_search_dev start. \n");
	
	pbiz_node = biz_find_or_add_node( cloud_id, shortid, endp_id );

	pbiz_node->waitting_dev_response = 1;


	ret = coord_search_dev(pbiz_node);	
	if( 0 != ret )
	{
		pbiz_node->waitting_dev_response = 0;
		return -1;
	}

	return 0;
}

int biz_search_dev_response(biz_node_t *pbiz_node, int success)
{
	pbiz_node->zb_biz_status = ZB_BIZ_PERMIT_JOIN_RESPED;
	if( success )
	{		
		return 0;
	}
	else
	{
		return -1;
	}
}

// each announce may create a new Node.
int biz_search_dev_dev_announce(biz_node_t *pbiz_node, zb_uint8 *payload, zb_uint8 len)
{
	zb_uint16 short_id;
	zb_uint8 *mac;
	zb_uint8 capability;

	short_id = *(zb_uint16 *)payload;
	mac = payload + 2;
	capability = *(mac + 8);
	
	zb_dev_add( short_id, mac, capability);
}

int biz_search_dev_read_id(biz_node_t *pbiz_node)
{
	
}






/***************** Biz node management **************************************/

biz_node_mgmt_t gBizNodes;

int biz_node_mgmt_init(void)
{
	int i=0;
	
	memset( &gBizNodes, 0, sizeof(biz_node_mgmt_t));

	for(i=0; i<BIZ_NODE_NUM; i++)
		INIT_LIST_HEAD( &gBizNodes.list[i] );

	init_session_with_coord();

	return 0;
}

biz_node_t * biz_add_node(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id)
{
	biz_node_t *pnode;
	zb_uint8 	hash;
	struct list_head  *head;
	
	hash = BIZ_NODE_HASH(cloud_id, shortid, endp_id);
	head = &gBizNodes.list[hash];
	
	pnode = (biz_node_t *)malloc(sizeof(biz_node_t));
	if( NULL == pnode)
	{
		return NULL;
	}

	memset(pnode, 0, sizeof(biz_node_t));
	INIT_LIST_HEAD( &pnode->list );	
	INIT_LIST_HEAD( &pnode->search_list);

	pnode->cloud_id = cloud_id;
	pnode->dev_shortid = shortid;
	pnode->dev_endp_id = endp_id;

	list_add(&pnode->list, head);

	ZB_DBG("---->biz_add_node: couldid:%x shortId:%x endpId:%x hash=0x%x\n", pnode->cloud_id, pnode->dev_shortid, pnode->dev_endp_id, hash);

	return pnode;
}

biz_node_t * biz_find_node(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id)
{
	zb_uint8 hash;
	biz_node_t *pos, *n;
	struct list_head *head;
	
	hash = BIZ_NODE_HASH(cloud_id, shortid, endp_id);
	head = &gBizNodes.list[hash];

	list_for_each_entry_safe(pos,n,head,list)
	{
		if(endp_id == 0)	//in this case, we didn't check endpoint id.
		{ 
			if( (pos->cloud_id == cloud_id) && (pos->dev_shortid == shortid))  //add seq to match the node who send the cmd.
			{
				return pos;
			}
		}
		else if( (pos->cloud_id == cloud_id) && (pos->dev_shortid == shortid) && (pos->dev_endp_id == endp_id))
		{
			return pos;
		}
	}
	
	ZB_DBG("---->biz_find_node, Not Found. cloud_id:%x shortid:%x endpId:%x hash=0x%x\n", cloud_id, shortid, endp_id, hash);

	return NULL;	
}

//If node not exists, create one.
biz_node_t * biz_find_or_add_node(zb_uint8 cloud_id, zb_uint16 shortid, zb_uint8 endp_id)
{
	biz_node_t * pnode;
	
	pnode = biz_find_node(cloud_id, shortid, endp_id);
	if( pnode )
	{
		return pnode;
	}

	return biz_add_node(cloud_id, shortid, endp_id);
}


/*******************************************************/



