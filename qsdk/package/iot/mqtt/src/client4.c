#include <pthread.h>
#include "client_shared.h"
#include "parse.h"
#include "msgque.h"



#define MAXSIZE_LINE 100
/*client4: >> cloud.config >> keepalive */

pthread_cond_t		cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t		mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t tid[3];

struct mosquitto *mosq_ext;

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	printf("mqtt connect success.\n");
	struct mosq_config *cfg;
	cfg = (struct mosq_config *)obj;
	if(rc){
		exit(1);
	}else{
		if(mosquitto_subscribe(mosq, NULL, cfg->ext_topic_config, 0)){
			printf("mosquitto_subscribe error\n");
		}
	}
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
	printf("mqtt dis-connect.\n");
	//run = rc;
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	int i;
	struct mosq_config *cfg;
	cfg = (struct mosq_config *)obj;

	if(!cfg->quiet) printf("Subscribed (mid: %d): %d", mid, granted_qos[0]);
	for(i=1; i<qos_count; i++){
		if(!cfg->quiet) printf(", %d", granted_qos[i]);
	}
	if(!cfg->quiet) printf("\n");
}

void on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}

void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	printf("\nmqtt recv:[ %s:%s ] and setting keepalive_user time\n",message->topic,(char*)message->payload);
	
	struct mosq_config *cfg;
	cfg = (struct mosq_config *)obj;

	if(parse(message->payload) !=3 ){
		printf("method is not keepalive, error\n");
	}else{
		//pthread_mutex_lock(&mutex);
		cfg->keepalive_user = getparams();
		//pthread_mutex_unlock(&mutex);
	}
}

void thread_recv_config(struct mosquitto *mosq)
{
	int rc;
	rc = mosquitto_loop_forever(mosq, -1, 1);
	if(rc){
		fprintf(stderr, "Error: mosquitto_loop_forever()\n");
	}
}

void thread_send_keepalive(struct mosq_config *cfg)
{
	char buf[100];
	sleep(1);
	while(1){
		sprintf(buf, "{\"method\":\"keepalive\",\"params\":{\"interval\":%d}}", cfg->keepalive_user);
		//printf("buf=[%s]\n",buf);
		//pthread_mutex_lock(&mutex);
		if(mosquitto_publish(mosq_ext, NULL, cfg->ext_topic_keepalive, strlen(buf), buf, 0, false)){
			printf("mosquitto_publish fail\n");
		}
		//pthread_mutex_lock(&mutex);
		printf("keepalive waitting(%d)\n",cfg->keepalive_user);
		sleep(cfg->keepalive_user);
	}
}

void thread_device_inform(struct mosq_config *cfg)
{
	char buf[1024];
	int queue_id;
	IOT_MSG send_msg;
	char line[MAXSIZE_LINE];
	FILE *fp;

	double tx=0;
	static double tx_old=0;
	int tx_speed=0;
	double rx=0;
	static double rx_old=0;
	int rx_speed=0;
	int subdevice_count=0;

	sleep(1);
	while(1){
#ifdef OPENWRT

	    	fp = fopen("/sys/devices/soc.0/c080000.edma/net/eth0/statistics/tx_bytes","r");
	    	//fp = fopen("tx_bytes","r");
    		if(fp==NULL){
			printf("fopen file 'tx_bytes' error! (%s)\n",__FILE__);
			continue;
		}
		fgets(line,MAXSIZE_LINE,fp);
    		fclose(fp);	
            	tx = strtod(line,NULL);

	    	fp = fopen("/sys/devices/soc.0/c080000.edma/net/eth0/statistics/rx_bytes","r");
	    	//fp = fopen("rx_bytes","r");
    		if(fp==NULL){
			printf("fopen file 'rx_bytes' error! (%s)\n",__FILE__);
			continue;
		}
		fgets(line,MAXSIZE_LINE,fp);
    		fclose(fp);	
            	rx = strtod(line,NULL);

		//printf("tx=[%lf] rx=[%lf]\n",tx,rx);

		if(tx_old){
			tx_speed = (int)(tx - tx_old)/cfg->device_inform;
		}else{
			tx_speed = 0;
		}
		tx_old = tx;
		if(rx_old){
			rx_speed = (int)(rx - rx_old)/cfg->device_inform;
		}else{
			rx_speed = 0;
		}
		rx_old = rx;

		//read device_count
		fp = fopen("/proc/net/arp","r");
    		if(fp==NULL){
			printf("fopen '/proc/net/arp' error! (%s)\n",__FILE__);
			continue;
		}
		subdevice_count=0;
		while(fgets(line,MAXSIZE_LINE,fp)){
        		//printf("%s\n",line);
			if(strstr(line,"eth0")){continue;}
			if(strstr(line,"0x0")){continue;}
			subdevice_count++;
		}		
    		fclose(fp);

#endif

		sprintf(buf, "{\"method\":\"DeviceNetworkSpeed\",\"params\":{\"up_speed\":%d,\"down_speed\":%d,\"subdevice_count\":%d}}", tx_speed, rx_speed, subdevice_count);
		printf("buf=[%s] sending to msgque",buf);

		//send to msgque	
		queue_id = iot_msg_queue_get(IOT_MODULE_EVENT_LOOP);
		if(queue_id == -1){
			printf("iot_msg_queue_get error!\n");
		}
		printf(" .");
		send_msg.dst_mod = IOT_MODULE_MQTT;
		send_msg.src_mod = IOT_MODULE_MQTT;
		send_msg.handler_mod = IOT_MODULE_NONE;
		strncpy(send_msg.body, buf, IOT_MSG_BODY_LEN_MAX);
		send_msg.body_len = strlen(send_msg.body)+1;
		printf(" .");
		if(iot_msg_send(queue_id, &send_msg)){
			printf("iot_msg_send error!\n");
		}
		printf(" .\n");

		sleep(cfg->device_inform);
	}
}

int main(int argc, char* argv[])
{
	int rc;
	struct mosq_config cfg;
//	struct mosquitto *mosq_ext;
	char name[100];

	rc = client_config_load(&cfg, CLIENT_SUB, argc, argv);
	if(rc){
		client_config_cleanup(&cfg);
		if(rc == 2){
			print_usage();
		}else{
			fprintf(stderr, "\nUse 'client4 --help' to see usage.\n");
		}
		return 1;
	}

	if(!cfg.keepalive_user){
		printf("please set mqtt.conf with --keepalive_user\n");
		return 1;
	}

	mosquitto_lib_init();

	sprintf(name,"client4_%d",getpid());
	mosq_ext = mosquitto_new(name, cfg.clean_session, &cfg);
	if(client_opts_set(mosq_ext, &cfg)){
		printf("client_opts_set error\n");
		return 1;
	}
	mosquitto_connect_callback_set(mosq_ext, on_connect);
	mosquitto_disconnect_callback_set(mosq_ext, on_disconnect);
	mosquitto_subscribe_callback_set(mosq_ext, on_subscribe);
        mosquitto_log_callback_set(mosq_ext, on_log);
	mosquitto_message_callback_set(mosq_ext, on_message);

	rc = mosquitto_connect(mosq_ext, cfg.ext_host, cfg.ext_port, 60);
	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		//return rc;
	}

//	rc = mosquitto_loop_forever(mosq_ext, -1, 1);

        pthread_create(&tid[0],NULL,(void*)thread_recv_config,(void*)mosq_ext);
        printf("  | [%ld]pthread_create(recv_config)\n", tid[0]);
        pthread_create(&tid[1],NULL,(void*)thread_send_keepalive,(void*)&cfg);
        printf("  | [%ld]pthread_create(send_keepalive)\n", tid[1]);
        pthread_create(&tid[2],NULL,(void*)thread_device_inform,(void*)&cfg);
        printf("  | [%ld]pthread_create(device_inform)\n", tid[2]);
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
	pthread_join(tid[2],NULL);



	//never reach
	mosquitto_destroy(mosq_ext);
	mosquitto_lib_cleanup();

	return rc;
}

