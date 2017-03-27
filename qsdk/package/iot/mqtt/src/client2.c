//#include "public.h"
//#include "mqtt.h"
#include "client_shared.h"
#include "parse.h"
#include "msgque.h"

/*client2: mqtt -> msg */

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	struct mosq_config *cfg;
	cfg = (struct mosq_config *)obj;
	printf("mqtt connect success.\n");
	if(rc){
		exit(1);
	}else{
		if(cfg->loc_topic_in){
			if(mosquitto_subscribe(mosq, NULL, cfg->loc_topic_in, 0)){
				printf("mosquitto_subscribe error\n");
			}
		}else{
			printf("please set the mqtt.conf with --loc_topic_in\n");
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

	assert(obj);
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
	printf("\nmqtt recv:[ %s:%s ] and device/in >> msgsend\n",message->topic,(char*)message->payload);
	msgsend(message->payload);	
	//parse((char*)message->payload);
}

int main(int argc, char* argv[])
{
	int rc;
	struct mosq_config cfg;
	struct mosquitto *mosq = NULL;

	rc = client_config_load(&cfg, CLIENT_SUB, argc, argv);
	if(rc){
		client_config_cleanup(&cfg);
		if(rc == 2){
			print_usage();
		}else{
			fprintf(stderr, "\nUse 'client2 --help' to see usage.\n");
		}
		return 1;
	}

	mosquitto_lib_init();

	mosq = mosquitto_new("client2", cfg.clean_session, &cfg);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!cfg.quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!cfg.quiet) fprintf(stderr, "Error: Invalid id and/or clean_session.\n");
				break;
		}
		mosquitto_lib_cleanup();
		printf("mosquitto_new fail\n");
		return 1;
	}
	if(client_opts_set(mosq, &cfg)){
		printf("client_opts_set error\n");
		return 1;
	}
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_subscribe_callback_set(mosq, on_subscribe);
        mosquitto_log_callback_set(mosq, on_log);
	mosquitto_message_callback_set(mosq, on_message);

	//rc = client_connect(mosq, &cfg);
	//rc = mosquitto_connect(mosq, "localhost", 1883, 60);
	rc = mosquitto_connect(mosq, cfg.loc_host, cfg.loc_port, 60);
	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return rc;
	}

	rc = mosquitto_loop_forever(mosq, -1, 1);

	//never reach
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return rc;
}

