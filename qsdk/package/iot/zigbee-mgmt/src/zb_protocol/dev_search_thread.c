/****************** Thread to search/add dev  ***********************************/
#include "biz_msg_parser.h"
#include "kernel_list.h"
#include "dev_search_thread.h"

//#include "biz_msg_process.h"
#include "zb_serial_protocol.h"

#include <sys/time.h>
#include <time.h>


#define MAX_NODE_NUM  256

struct thread_dev_search
{	
	struct list_head dev_nodes;	
	pthread_mutex_t list_lock;	
	
	pthread_cond_t 	stat_cond;	
	pthread_mutex_t stat_mutex;	

	pthread_t	thread_id;
};

struct thread_dev_search gThreadDevSearch;


int init_dev_search_thread(void)
{
	memset(&gThreadDevSearch, 0, sizeof(struct thread_dev_search));
	
	INIT_LIST_HEAD( &gThreadDevSearch.dev_nodes );
	pthread_mutex_init( &gThreadDevSearch.list_lock, NULL);	

	pthread_cond_init(  &gThreadDevSearch.stat_cond, NULL );
	pthread_mutex_init( &gThreadDevSearch.stat_mutex, NULL);	

	return 0;
}

// 200ms
#define DEV_SEARCH_THREAD_TIMEOUT_NS 1000*1000*200

#if 0
void *dev_search_thread_func(void *arg)
{
	struct thread_dev_search *pThreadDevS = (struct thread_dev_search *)arg;
	int rc;
	struct timespec absTime;
	biz_node_t *pos, *n;
	int processed;
	
	int exit_list_loop = 0;
	
	ZB_DBG("dev_search_thread_func start.\n ");
	
	while(1)
	{
		processed = 0;
		
		if( list_empty(&gThreadDevSearch.dev_nodes) )
		{	
			clock_gettime(CLOCK_REALTIME, &absTime);		
			absTime.tv_nsec += DEV_SEARCH_THREAD_TIMEOUT_NS;
			absTime.tv_sec += absTime.tv_nsec/1000000000;
			absTime.tv_nsec %= 1000000000;

			pthread_mutex_lock(&gThreadDevSearch.stat_mutex);
			
			do{
				rc = pthread_cond_timedwait( &gThreadDevSearch.stat_cond, &gThreadDevSearch.stat_mutex, &absTime);	
						
			}while( (rc==0) && (list_empty(&gThreadDevSearch.dev_nodes)) ); //wakeup unexpected.

			pthread_mutex_unlock( &gThreadDevSearch.stat_mutex );
		}

		// 1. remove node from list.		
		pthread_mutex_lock(&gThreadDevSearch.list_lock);

		if( !list_empty(&gThreadDevSearch.dev_nodes))
		{
			pos = list_entry( gThreadDevSearch.dev_nodes.next, biz_node_t, search_list);

			list_del( &pos->search_list );
		}
		else
		{
			pos = NULL;
		}
		pthread_mutex_unlock(&gThreadDevSearch.list_lock);

		if( pos == NULL)
		{
			continue;
		}		

		// 2. process base on the status of this node.		
		switch(pos->zb_biz_status)
		{				
			case ZB_BIZ_GOT_DEVICE_ANNOUNCE:
			{
				//send attribute.
				zb_dev_get_id(pos);
				pos->zb_biz_status = ZB_BIZ_READING_ID;
				break;
			}
			case ZB_BIZ_GOT_ID:
			{
				//trigger bind request.
				//For light, bind STATUS.
				//get dev's mac id by shortid.
				//coord_bind_req( zb_dev_get_mac_by_short_id(pos->dev_shortid),
				//	pos->dev_shortid, pos->dev_endp_id);
				
				zb_dev_bind_request(pos, 
					pos->dev_mac,
					pos->dev_endp_id, 
					0x6);	//cluster id.
					
				pos->zb_biz_status = ZB_BIZ_BIND_SENT;
				
				ZB_DBG("!!!! pos:0x%p  status:ZB_BIZ_BIND_SENT.\n", pos);
				break;
			}
			case ZB_BIZ_BIND_RESP:
			{
				//zb_send_config_report.
				zb_dev_rpt_configure( pos );
				
				pos->zb_biz_status = ZB_BIZ_CONFIG_REPORTING_SENT;				
				break;
			}	
			case ZB_BIZ_CONFIG_REPORTING_RESP:
			{
				ZB_DBG("\n\n>> set configure reporting complete. <<\n");				
				break;
			}
			case ZB_BIZ_SEARCH_COMPLETE_DEL_IT:
			{				
				ZB_DBG("\n\n>> set configure reporting Fail.  Try kick device <<\n");	

				{
					int t=0;
					for(t=0; t<10; t++)
					{
						zb_onoff_light(pos, 0x02);	//toggle
						sleep(2);
					}
				}
				//zb_dev_leave_request(pos, pos->dev_mac, 0);
				break;
			}
		}
	}

	ZB_DBG("dev_search_thread_func exitd.\n ");
	
	return NULL;	
}
#endif

void *dev_search_thread_func_new(void *arg)
{
	struct thread_dev_search *pThreadDevS = (struct thread_dev_search *)arg;
	int rc;
	struct timespec absTime;
	struct serial_session_node *pos, *n;
	int processed;
	
	int exit_list_loop = 0;
	
	ZB_DBG("dev_search_thread_func start.\n ");
	
	while(1)
	{
		processed = 0;
		
		if( list_empty(&gThreadDevSearch.dev_nodes) )
		{	
			clock_gettime(CLOCK_REALTIME, &absTime);		
			absTime.tv_nsec += DEV_SEARCH_THREAD_TIMEOUT_NS;
			absTime.tv_sec += absTime.tv_nsec/1000000000;
			absTime.tv_nsec %= 1000000000;

			pthread_mutex_lock(&gThreadDevSearch.stat_mutex);
			
			do{
				rc = pthread_cond_timedwait( &gThreadDevSearch.stat_cond, &gThreadDevSearch.stat_mutex, &absTime);	
						
			}while( (rc==0) && (list_empty(&gThreadDevSearch.dev_nodes)) ); //wakeup unexpected.

			pthread_mutex_unlock( &gThreadDevSearch.stat_mutex );
		}

		// 1. remove node from list.		
		pthread_mutex_lock(&gThreadDevSearch.list_lock);

		if( !list_empty(&gThreadDevSearch.dev_nodes))
		{
			pos = list_entry( gThreadDevSearch.dev_nodes.next, struct serial_session_node, search_list);

			list_del( &pos->search_list );
		}
		else
		{
			pos = NULL;
		}
		pthread_mutex_unlock(&gThreadDevSearch.list_lock);

		if( pos == NULL)
		{
			continue;
		}		

		// 2. process base on the status of this node.		
		switch(pos->zb_join_status)
		{				
			case ZB_BIZ_GOT_DEVICE_ANNOUNCE:
			{
				//send attribute.
				host_send_get_ids(pos);
				pos->zb_join_status = ZB_BIZ_READING_ID;
				break;
			}
			case ZB_BIZ_GOT_ID:
			{
				//trigger bind request.
				//For light, bind STATUS.
				//get dev's mac id by shortid.
				//coord_bind_req( zb_dev_get_mac_by_short_id(pos->dev_shortid),
				//	pos->dev_shortid, pos->dev_endp_id);
				#if 0
				host_send_bind_request(pos, 
					pos->mac,
					pos->dst_addr.endp_addr, 
					0x6);	//cluster id.
				#endif

				//test, bind colorX,colorY
				host_send_bind_request(pos, 
					pos->mac,
					pos->dst_addr.endp_addr, 
					0x0300);	//cluster id.
					
				pos->zb_join_status = ZB_BIZ_BIND_SENT;
				
				ZB_DBG("!!!! pos:0x%p  status:ZB_BIZ_BIND_SENT.\n", pos);
				break;
			}
			case ZB_BIZ_BIND_RESP:
			{
				//zb_send_config_report.
				host_send_rpt_configure( pos );
				
				pos->zb_join_status = ZB_BIZ_CONFIG_REPORTING_SENT;				
				break;
			}	
			case ZB_BIZ_CONFIG_REPORTING_RESP:
			{
				ZB_DBG("\n\n>> set configure reporting complete. <<\n");	
				pos->zb_join_status = ZB_BIZ_SEARCH_COMPLETE;

				
				//TODO: If all endpoint of a device is complete,
				//move this device to working list. Do it in coord_msg_handle(),
				
				break;
			}
			case ZB_BIZ_SEARCH_COMPLETE_DEL_IT:
			{				
				ZB_DBG("\n\n>> KICK it. <<\n");	

				host_send_dev_leave_request(pos, pos->mac, 0);
				
				#if 0
				{
					//debug only
					int t=0;
					for(t=0; t<10; t++)
					{
						zb_onoff_light_new(pos, 0x02);	//toggle
						sleep(2);
					}
				}
				#endif
				
				break;
			}
		}
	}

	ZB_DBG("dev_search_thread_func exitd.\n ");
	
	return NULL;	
}

void add_node_to_search_list_new(struct serial_session_node * pnode)
{
	//ZB_DBG("---Add node to search list begin--\n");
	
	pthread_mutex_lock( &gThreadDevSearch.list_lock);
	list_add( &pnode->search_list, &gThreadDevSearch.dev_nodes);	
	pthread_mutex_unlock( &gThreadDevSearch.list_lock );

	//wake up thread if required.
	pthread_mutex_lock( &gThreadDevSearch.stat_mutex);	
	pthread_cond_signal( &gThreadDevSearch.stat_cond );	
	pthread_mutex_unlock( &gThreadDevSearch.stat_mutex);	
	
	//ZB_DBG("---wakeup search thread end--\n");
}



int start_dev_search_thread(void)
{
	int ret;
	ret  = pthread_create( &gThreadDevSearch.thread_id, NULL, dev_search_thread_func_new, &gThreadDevSearch);

	if( 0 != ret)
	{
		ZB_ERROR("Failed to create Thread : start_dev_search_thread.");
		return -1;
	}
	ZB_DBG(">> start_dev_search_thread created.\n");

	return 0;
}



