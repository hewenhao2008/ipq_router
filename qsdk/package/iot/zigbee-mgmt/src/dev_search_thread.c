/****************** Thread to search/add dev  ***********************************/
#include "biz_msg_parser.h"
#include "kernel_list.h"
#include "dev_search_thread.h"
#include "biz_msg_process.h"
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
		
		exit_list_loop = 0;
		
		//process each node in the list.
		list_for_each_entry_safe(pos, n, &gThreadDevSearch.dev_nodes, search_list)
		{
			// 1. timeout handle

			// 2. normal status process.
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
						zb_dev_get_mac_by_short_id(pos->dev_shortid),
						pos->dev_shortid, pos->dev_endp_id);
						
					pos->zb_biz_status = ZB_BIZ_BIND_SENT;
				}
				case ZB_BIZ_BIND_RESP:
				{
					pos->zb_biz_status = ZB_BIZ_SEARCH_COMPLETE;
					//break. we think it is complete.
					list_del( &pos->search_list );
					exit_list_loop= 1;
					break;
				}	
				case ZB_BIZ_SEARCH_COMPLETE_DEL_IT:
				{
					list_del( &pos->search_list );
					exit_list_loop = 1;
					break;
				}
			}

			if(exit_list_loop)
			{
				break; //exit list_for_each loop. since we del entry in the loop, it is not safe to scan it now.	
			}
		}//end of list_for_each


		
	}

	ZB_DBG("dev_search_thread_func exitd.\n ");
	
	return NULL;	
}
#endif

void add_node_to_search_list(biz_node_t * pnode)
{
	//ZB_DBG("---Add node to search list begin--\n");
	
	pthread_mutex_lock( &gThreadDevSearch.list_lock);
	list_add( &pnode->search_list, &gThreadDevSearch.dev_nodes);	
	pthread_mutex_unlock( &gThreadDevSearch.list_lock );

	//ZB_DBG("---Add node to search list end--\n");

	//ZB_DBG("---wakeup search thread begin--\n");

	//wake up thread if required.
	pthread_mutex_lock( &gThreadDevSearch.stat_mutex);	
	pthread_cond_signal( &gThreadDevSearch.stat_cond );	
	pthread_mutex_unlock( &gThreadDevSearch.stat_mutex);	
	
	//ZB_DBG("---wakeup search thread end--\n");
}


int start_dev_search_thread(void)
{
	int ret;
	ret  = pthread_create( &gThreadDevSearch.thread_id, NULL, dev_search_thread_func, &gThreadDevSearch);

	if( 0 != ret)
	{
		ZB_ERROR("Failed to create Thread : start_dev_search_thread.");
		return -1;
	}
	ZB_DBG(">> start_dev_search_thread created.\n");

	return 0;
}



