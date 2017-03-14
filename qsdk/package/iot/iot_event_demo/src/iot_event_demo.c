#include "iot_event_demo.h"
#include "mqtt_demo_thread.h"

static void clear_msg_queue(int queue_id, unsigned char dst_mod)
{
	int res;
	IOT_MSG msg;
	while(1){
		msg.dst_mod = dst_mod;
		res = iot_msg_recv_nowait(queue_id, &msg);
		if(res<0)
			break;
	}
}

static int wifi_subdev_mgr_demo(int argc, char *argv[])
{
	int res;
	int queue_id;
	IOT_MSG db_msg, wifi_msg, zigbee_msg, send_msg;
	
	queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
	if(queue_id == -1)
	{
		fprintf(stderr, "wifi_subdev_mgr : call iot_msg_queue_get failed, res=%d, err:%s\n", 
							queue_id,strerror(errno));
		return 1;
	}

	clear_msg_queue(queue_id, IOT_MODULE_DATABASE);
	clear_msg_queue(queue_id, IOT_MODULE_WIFI);
	clear_msg_queue(queue_id, IOT_MODULE_ZIGBEE);

	db_msg.dst_mod = IOT_MODULE_DATABASE;
	wifi_msg.dst_mod = IOT_MODULE_WIFI;
	zigbee_msg.dst_mod = IOT_MODULE_ZIGBEE;
	while(1)
	{
		int get = 0;
		res = iot_msg_recv_nowait(queue_id, &db_msg);
		if(res<0)
		{
			if(errno == ENOMSG)
			{
				//fprintf(stderr, "Database msg : iot_msg_recv_nowait no msg get\n");
			}else
			{
				fprintf(stderr, "Database : call iot_msg_recv_nowait failed, res=%d, err:%s\n", res,strerror(errno));
			}
		}else
		{	
			get = 1;
			fprintf(stderr, "Database : receive a msg, res=%d, src_mod=%d, dst_mod=%d, body_len=%d, body=\n\n%s\n\n", 
					res, db_msg.src_mod, db_msg.handler_mod, db_msg.body_len, db_msg.body);

			send_msg.dst_mod = IOT_MODULE_EVENT_LOOP;
			send_msg.src_mod = IOT_MODULE_DATABASE;
			send_msg.handler_mod = IOT_MODULE_NONE;
			strncpy(send_msg.body, "{ \"result\" = \"ok\" }", IOT_MSG_BODY_LEN_MAX);
			send_msg.body_len = strlen(send_msg.body)+1;
			iot_msg_send(queue_id, &send_msg);
		}

		res = iot_msg_recv_nowait(queue_id, &wifi_msg);
		if(res<0)
		{
			if(errno == ENOMSG)
			{
				//fprintf(stderr, "Wifi msg : iot_msg_recv_nowait no msg get\n");
			}else
			{
				fprintf(stderr, "Wifi : call iot_msg_recv_nowait failed, res=%d, err:%s\n", res,strerror(errno));
			}
		}else
		{	
			get = 1;
			fprintf(stderr, "Wifi : receive a msg, res=%d, src_mod=%d, dst_mod=%d, body_len=%d, body=\n\n%s\n\n", 
					res, wifi_msg.src_mod, wifi_msg.handler_mod, wifi_msg.body_len, wifi_msg.body);

			send_msg.dst_mod = IOT_MODULE_EVENT_LOOP;
			send_msg.src_mod = IOT_MODULE_WIFI;
			send_msg.handler_mod = IOT_MODULE_NONE;
			strncpy(send_msg.body, "{ \"result\" = \"ok\" }", IOT_MSG_BODY_LEN_MAX);
			send_msg.body_len = strlen(send_msg.body)+1;
			iot_msg_send(queue_id, &send_msg);

		}

		res = iot_msg_recv_nowait(queue_id, &zigbee_msg);
		if(res<0)
		{
			if(errno == ENOMSG)
			{
				//fprintf(stderr, "Wifi msg : iot_msg_recv_nowait no msg get\n");
			}else
			{
				fprintf(stderr, "Zigbee : call iot_msg_recv_nowait failed, res=%d, err:%s\n", res,strerror(errno));
			}
		}else
		{	
			get = 1;
			fprintf(stderr, "Zigbee : receive a msg, res=%d, src_mod=%d, dst_mod=%d, body_len=%d, body=\n\n%s\n\n", 
					res, zigbee_msg.src_mod, zigbee_msg.handler_mod, zigbee_msg.body_len, zigbee_msg.body);
			
			send_msg.dst_mod = IOT_MODULE_EVENT_LOOP;
			send_msg.src_mod = IOT_MODULE_ZIGBEE;
			send_msg.handler_mod = IOT_MODULE_NONE;
			strncpy(send_msg.body, "{ \"result\" = \"ok\" }", IOT_MSG_BODY_LEN_MAX);
			send_msg.body_len = strlen(send_msg.body)+1;
			iot_msg_send(queue_id, &send_msg);
		}

		if(get==0)
			sleep(2);
	}
	
	return 0;
}

static void sigterm(int signo)
{
	mqtt_demo_thread_exit();
    exit(0);
}

int main(int argc, char *argv[])
{
    if(signal(SIGTERM,sigterm)==SIG_ERR ||
    			signal(SIGINT,sigterm)==SIG_ERR ){
        fprintf(stderr,"signal for SIGTERM or SIGINT error:%s\n",strerror(errno));
        return 1;
    }
    mqtt_demo_thread_prepare();


	wifi_subdev_mgr_demo(argc, argv);

    mqtt_demo_thread_exit();
    
    return 0;
}
