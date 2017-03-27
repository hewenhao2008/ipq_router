//#include "public.h"
#include "client_shared.h"
#include "parse.h"
#include "msgque.h"

/*client3: mqtt_cloud -> mqtt_local */


struct mosquitto *mosq_loc;

int mqtt_pub_client3(struct mosq_config *cfg, char* buf)
{
	int rc;
	if(mosquitto_connect(mosq_loc, cfg->loc_host, cfg->loc_port, 60)){
		printf("mosquitto_connect fail\n");
		rc = 1;
	}		
	if(mosquitto_publish(mosq_loc, NULL, cfg->loc_topic_in, strlen(buf), buf, 0, false)){
		printf("mosquitto_publish fail\n");
		rc = 2;
	}
	mosquitto_disconnect(mosq_loc);
	return rc;
}

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	printf("mqtt connect success.\n");
	struct mosq_config *cfg;
	cfg = (struct mosq_config *)obj;
	if(rc){
		exit(1);
	}else{
		if(mosquitto_subscribe(mosq, NULL, cfg->ext_topic_in, 0)){
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
	printf("mqtt recv:[ %s:%s ] and device_id/in >> device/in (cloud >> local)\n",message->topic,(char*)message->payload);
	
	struct mosq_config *cfg;
	cfg = (struct mosq_config *)obj;

	mqtt_pub_client3(cfg, message->payload);	
}

int main(int argc, char* argv[])
{
	int rc;
	struct mosq_config cfg;
	struct mosquitto *mosq_ext;
//	struct mosquitto *mosq_loc;

	rc = client_config_load(&cfg, CLIENT_SUB, argc, argv);
	if(rc){
		client_config_cleanup(&cfg);
		if(rc == 2){
			print_usage();
		}else{
			fprintf(stderr, "\nUse 'client3 --help' to see usage.\n");
		}
		return 1;
	}

	mosquitto_lib_init();

	mosq_loc = mosquitto_new("client3", cfg.clean_session, &cfg);
	mosq_ext = mosquitto_new("client3_idxxx", cfg.clean_session, &cfg);
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
		return rc;
	}

	rc = mosquitto_loop_forever(mosq_ext, -1, 1);

	//never reach
	mosquitto_destroy(mosq_ext);
	mosquitto_destroy(mosq_loc);
	mosquitto_lib_cleanup();

	return rc;
}

