#include "mqtt_demo_thread.h"

static pthread_t g_deal_thread;
static int g_thread_err = -1;
static int g_thread_exit_flag = 0;

static char * test_cmd[] = 
{
	"{ \
    	\"method\": \"getRoomDevices\", \
    	\"params\": { \
        	\"room\": 1 \
    	}, \
    	\"id\": 1 \
	}",

	"{ \
    	\"method\": \"getWLANDevices\", \
    	\"params\": { \
    	}, \
    	\"id\": 1 \
	}", 

	"{ \
	    \"method\": \"getDeviceNetworkSpeed\", \
	    \"params\": { \
	    	\"device_uuid\":\"42323\" \
	    }, \
	    \"id\": 1 \
	}", 
	
	"{ \
	    \"method\": \"setDeviceAccess\", \
	    \"params\": { \
	    	\"device_uuid\":\"12323\", \
	    	\"enable\":true \
	    }, \
	    \"id\": 1 \
	}",
	
	"{ \
	    \"method\": \"getDeviceProperty\", \
	    \"params\": { \
	        \"device_uuid\": \"52323FE\" \
	    }, \
	    \"id\": 1 \
	}",  

	"{ \
    \"client\": \"android\", \
    \"timestamp\": 1487236771, \
    \"version\": 1.1, \
    \"client_version\":\"android5.0\", \
    \"user_agent\": \"huawei\", \
    \"method\": \"control\", \
    \"token\": \"adfljasdklfjawefmdfamsdfklajdf\", \
    \"params\": { \
        \"cmd\": \"write\", \
        \"model\": \"qualcomm.plug.v1\", \
        \"device_uuid\": \"858d0000e7ccfd\", \
        \"token\": \"5\", \
        \"short_id\": 42653, \
        \"data\": \"{\\\"status\\\":\\\"toggle\\\"}\" \
     } \
	} ",

	NULL,
};

static void * thr_deal(void*arg)
{
	int res;
	int i=0;
	int queue_id;
	IOT_MSG msg_buf;
		
	while(g_thread_exit_flag==0){

		queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
		if(queue_id == -1)
		{
			fprintf(stderr, "mqtt_demo_thread: call iot_msg_queue_get failed, res=%d, err:%s\n", queue_id,strerror(errno));
			sleep(1);
			continue;
		}
		
		if(test_cmd[i] == NULL)
			break;

		msg_buf.dst_mod = IOT_MODULE_EVENT_LOOP;
		msg_buf.handler_mod = IOT_MODULE_NONE;
		msg_buf.src_mod = IOT_MODULE_MQTT;
		strncpy(msg_buf.body, test_cmd[i], IOT_MSG_BODY_LEN_MAX);
		msg_buf.body_len = strlen(msg_buf.body)+1;

		i++;
		res = iot_msg_send(queue_id, &msg_buf);
		if(res<0)
		{
			fprintf(stderr, "mqtt : call iot_msg_send failed, res=%d, err:%s\n", res,strerror(errno));
			sleep(1);
			continue;
		}
		
		msg_buf.dst_mod = IOT_MODULE_MQTT;
		res = iot_msg_recv_wait(queue_id, &msg_buf);
		if(res<0)
		{
			fprintf(stderr, "mqtt : call iot_msg_recv_wait failed, res=%d, err:%s\n", res,strerror(errno));
			continue;
		}
		
		fprintf(stderr, "mqtt : receive a msg, res=%d, src_mod=%d, dst_mod=%d, body_len=%d, body=\n\n%s\n\n", 
					res, msg_buf.src_mod, msg_buf.handler_mod, msg_buf.body_len, msg_buf.body);

			
		sleep(1);
	}

	return 0;
}

void mqtt_demo_thread_prepare()
{
	g_thread_err = -1;
	g_thread_exit_flag = 0;

	g_thread_err = pthread_create(&g_deal_thread,NULL,thr_deal,NULL);
	if(g_thread_err!=0){
		g_deal_thread = -1;
		fprintf(stderr,"Create deal thread error:%s\n",strerror(errno));
		return;
	}
}

void mqtt_demo_thread_exit()
{
	int err;
	void *tret;
	if(g_thread_err!=0){
		return ;
	}

	g_thread_exit_flag = 1;
	err = pthread_join(g_deal_thread,&tret);
	if(err!=0){
		fprintf(stderr,"can't join with deal thread:%s\n",strerror(errno));
		pthread_kill(g_deal_thread, SIGQUIT);
	}

	g_thread_err = -1;
	g_thread_exit_flag = 0;
}





