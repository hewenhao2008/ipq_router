#include <pthread.h>
#include "client_shared.h"
#include "parse.h"
#include "msgque.h"

int main(int argc, char* argv[])
{
	int rc;
	struct mosq_config cfg;
	struct mosquitto *mosq_loc;
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

	mosquitto_lib_init();

	sprintf(name,"fake_app%d",getpid());
	mosq_loc = mosquitto_new(name, cfg.clean_session, &cfg);
	if(client_opts_set(mosq_loc, &cfg)){
		printf("client_opts_set error\n");
		return 1;
	}

	rc = mosquitto_connect(mosq_loc, cfg.loc_host, cfg.loc_port, 60);
	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
		return rc;
	}

	char buf[100];
	while(1){
		sprintf(buf, "{\"method\":\"control\",\"params\":{},\"id\":1}");
		printf("sending [%s]to %s\n",buf,cfg.loc_topic_in);
		if(mosquitto_publish(mosq_loc, NULL, cfg.loc_topic_in, strlen(buf), buf, 0, false)){
			printf("mosquitto_publish fail\n");
		}
		usleep(100000);
	}

	//never reach
	mosquitto_destroy(mosq_loc);
	mosquitto_lib_cleanup();

	return rc;
}

