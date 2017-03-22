#include "biz_msg_process.h"
#include "biz_msg_parser.h"
#include "kernel_list.h"

// 1 second get response.
#define CMD_SEARCH_DEV_TIMEOUT_NS	1000*1000*1000

// could_id = 0, shortid=0, endp_id = 0xF1 (COORD_ADDR_ENDP_ADDR);
int biz_search_dev(char* token, zb_uint8 *uuid, char * manufactureName, char *moduleId)
{
	biz_node_t *pbiz_node;	
	int ret;
	int cnt;
	struct timespec absTime;

	ZB_DBG(">> biz_search_dev start. \n");
	
	pbiz_node = biz_find_or_add_node( token );

	pbiz_node->uuid[0] = uuid[0];

	strncpy( pbiz_node->manufactureName, manufactureName, BIZ_MANUFACTURE_NAME_LEN);
	pbiz_node->manufactureName[BIZ_MANUFACTURE_NAME_LEN-1] = '\0';

	strncpy( pbiz_node->moduleId, moduleId, BIZ_MODULE_ID_LEN);
	pbiz_node->moduleId[BIZ_MODULE_ID_LEN-1] = '\0';
	
	pbiz_node->waitting_dev_response = 1;

	cnt = zb_dev_get_endpoints(manufactureName, moduleId,  pbiz_node->endpoint_addrs);
	if( cnt <= 0)
	{
		ZB_ERROR(">> !!! User Not configure the endpoints of this device. Can not add device.\n");
		return -1;
	}
	pbiz_node->endpoint_cnt = cnt;

	ret = host_send_cmd_permit_join(pbiz_node);	
	if( 0 != ret )
	{
		pbiz_node->waitting_dev_response = 0;
		return -1;
	}

	return 0;
}

int biz_search_dev_response(biz_node_t *pbiz_node, int success)
{
	
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

//configure whether we are doing device search.
biz_node_t * biz_get_node_doing_search()
{
	return gBizNodes.pg_biz_node_adding_dev;
}

void biz_set_node_doing_search(biz_node_t * pNode)
{
	gBizNodes.pg_biz_node_adding_dev= pNode;
}



biz_node_t * biz_add_node(char * token)
{
	biz_node_t *pnode;
	zb_uint8 	hash;
	struct list_head  *head;
	
	hash = BIZ_NODE_HASH(token);
	head = &gBizNodes.list[hash];
	
	pnode = (biz_node_t *)malloc(sizeof(biz_node_t));
	if( NULL == pnode)
	{
		return NULL;
	}

	memset(pnode, 0, sizeof(biz_node_t));
	INIT_LIST_HEAD( &pnode->list );	
	INIT_LIST_HEAD( &pnode->search_list);
	INIT_LIST_HEAD( &pnode->endp_chain );

	//pnode->token = token;
	strncpy(pnode->token , token, 64);

	list_add(&pnode->list, head);

	ZB_DBG("---->biz_add_node: token=%s pnode:%p\n", token, pnode);

	return pnode;
}

biz_node_t * biz_find_node(char * token)
{
	zb_uint8 hash;
	biz_node_t *pos, *n;
	struct list_head *head;
	
	hash = BIZ_NODE_HASH(token);
	head = &gBizNodes.list[hash];

	list_for_each_entry_safe(pos,n,head,list)
	{
		//if( pos->token == token)
		if( !strncmp(pos->token, token, 64))
		{
			return pos;
		}	
	}
	
	ZB_DBG("---->biz_find_node, Not Found. token=%x\n", token);

	return NULL;	
}

//If node not exists, create one.
biz_node_t * biz_find_or_add_node(char * token)
{
	biz_node_t * pnode;
	
	pnode = biz_find_node(token);
	if( pnode )
	{
		return pnode;
	}

	return biz_add_node(token);
}

/*******************************************************/



