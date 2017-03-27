//#include "public.h"
//#include "mqtt.h"
#include "client_shared.h"
#include "parse.h"
#include "msgque.h"

/*client1: msg -> mqtt */


int mqtt_pub_client1_loc(struct mosquitto *mosq, struct mosq_config *cfg, char* buf)
{
	int rc = 0;
	if(mosquitto_connect(mosq, cfg->loc_host, cfg->loc_port, 60)){
		printf("mosquitto_connect fail\n");
		rc = 1;
	}		
  //      printf("[%s] >> %s\n", buf, cfg->loc_topic_out);
	if(mosquitto_publish(mosq, NULL, cfg->loc_topic_out, strlen(buf), buf, 0, false)){
		printf("mosquitto_publish fail\n");
		rc = 2;
	}
	mosquitto_disconnect(mosq);
	return rc;
}

int mqtt_pub_client1_ext(struct mosquitto *mosq, struct mosq_config *cfg, char* buf)
{
	int rc = 0;
	if(mosquitto_connect(mosq, cfg->ext_host, cfg->ext_port, 60)){
		printf("mosquitto_connect fail\n");
		rc = 1;
	}		
//        printf("[%s] >> %s\n", buf, cfg->ext_topic_out);
	if(mosquitto_publish(mosq, NULL, cfg->ext_topic_out, strlen(buf), buf, 0, false)){
		printf("mosquitto_publish fail\n");
		rc = 2;
	}
	mosquitto_disconnect(mosq);
	return rc;
}


int main(int argc, char *argv[])
{  
	int rc;
	struct mosq_config cfg;
	struct mosquitto *mosq_ext = NULL;
	struct mosquitto *mosq_loc = NULL;

	char buf[IOT_MSG_BODY_LEN_MAX];

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

	if(!(cfg.loc_host && cfg.loc_port)){
		printf("please set the mqtt.conf with --loc_host --loc_port\n");
		return 1;
	}
	if(!(cfg.loc_topic_out)){
		printf("please set the mqtt.conf with --loc_topic_out\n");
		return 1;
	}

	mosquitto_lib_init();
	mosq_ext = mosquitto_new("client1_idxxx/out", true, NULL);
	mosq_loc = mosquitto_new("client1", true, NULL);

  

	//从队列中获取消息,然后通过mqtt发送 
	while(1){  
		printf("\nwaitting...\n");
		rc = msgrecv(buf);
		if(rc){
			fprintf(stderr, "msgrecv error!\n");
		}
		printf(">> [%s] and sending...\n", buf);

		rc = mqtt_pub_client1_loc(mosq_loc, &cfg, buf); 
		if(rc){
			fprintf(stderr, "mqtt_pub_client1_loc error!\n");
		}

		rc = mqtt_pub_client1_ext(mosq_ext, &cfg, buf); 
		if(rc){
			fprintf(stderr, "mqtt_pub_client1_ext error!\n");
		}
	}

	//never reach
        mosquitto_destroy(mosq_loc);
        mosquitto_destroy(mosq_ext);
	mosquitto_lib_cleanup();

	return 0;
}

