#include "parse.h"

char method[100];
int interval;
int id;
//char*dist[]={"aa","bb"};

char*dist[]={\
//getWanConfig
"{\n\
	\"timestamp\":\"1487316372\",\n\
	\"msg\":\"success\",\n\
	\"code\":200,\n\
	\"result\":{\n\
			\"data\":[\n\
				{\n\
					\"wan_config\":\"adsl\",\n\
				},\n\
				],\n\
		}\n\
}",\
"{\n\
	\"timestamp\":\"1487316372\",\n\
	\"msg\":\"Detection Failed\",\n\
	\"code\":404\n\
}",\
//control
"{\n\
	\"timestamp\":\"1487316372\",\n\
	\"msg\":\"success\",\n\
	\"code\":201,\n\
	\"result\":{\n\
		\"data\":[\n\
				{\n\
				\"cmd\":\"report\",\n\
				\"model\":\"qualcomm.plug.v1\",\n\
				\"device_uuid\":\"158d000e7ccfd\",\n\
				\"token\":\"5\",\n\
				\"short_id\":42653,\n\
				\"data\":{\"status\":\"on\"}\n\
				},\n\
			],\n\
		}\n\
}",\
"{\n\
	\"timestamp\":\"1487316372\",\n\
	\"msg\":\"failed\",\n\
	\"code\":405\n\
}",\
	};


				//\"data\":\"{\"status\":\"on\"}\"\n\

//int main(void)
int parse(const char* str)
{
	int rc;
	const char *input = "{\n\
		\"key1\": 123,\n\
		\"key2\": \"hahah\",\n\
		\"key3\": 1,\n\
	}";
	const char *input2 = "{\n\
		\"method\":\"getWanConfig\",\n\
		\"params\":\"{c1:1,c2:2,c3:3}\",\n\
		\"id\":1,\n\
	}";

	struct json_object *new_obj = json_tokener_parse(str);
	//struct json_object *new_obj = json_tokener_parse(input2);
	if (!new_obj){
		printf("json_tokener_parse error!\n");
		return 0; 
	}
	printf("json_object=%s\n",json_object_to_json_string(new_obj));

	struct json_object *sub_obj;
	struct json_object *thr_obj;

        if(!json_object_object_get_ex(new_obj,"id",&sub_obj)){
		printf("json_object_object_get_ex 'id' error!\n");
	}else{
		if(!json_object_is_type(sub_obj, json_type_int)){
			printf("id obj type error!\n");
		}
		id = json_object_get_int(sub_obj);
		printf("id:%d\n", id);
		json_object_put(sub_obj);
	}

        if(!json_object_object_get_ex(new_obj,"method",&sub_obj)){
		printf("json_object_object_get_ex 'method' error!\n");
	}else{
		if(!json_object_is_type(sub_obj, json_type_string)){
			printf("method obj type error!\n");
		}
		strcpy(method, json_object_get_string(sub_obj));
		json_object_put(sub_obj);
		printf("method:%s\n", method);
		if(0==strcmp(method,"getWanConfig")) rc = 1;
		if(0==strcmp(method,"control")) rc = 2;
		if(0==strcmp(method,"keepalive")){
        		if(!json_object_object_get_ex(new_obj,"params",&sub_obj)){
				printf("json_object_object_get_ex 'params' error!\n");
				rc = 0;
			}else{
				printf("params:%s\n", json_object_to_json_string(sub_obj));
        			if(!json_object_object_get_ex(sub_obj,"interval",&thr_obj)){
					printf("json_object_object_get_ex 'interval' error!\n");
					rc = 0;
				}
				if(!json_object_is_type(thr_obj, json_type_int)){
					printf("interval obj is not int type!\n");
					rc = 0;
				}else{
					interval = json_object_get_int(thr_obj);
					printf("interval:%d\n",interval);
					json_object_put(thr_obj);
				}
				json_object_put(sub_obj);
			}	
			rc = 3;
		}
	}
	json_object_put(new_obj);

	return rc;
}

int getparams(void)
{
	return interval;
}

int package(const int method_no, const int test_result, char*buf){
/*
	struct json_object *my_object = json_object_new_object();
	json_object_object_add(my_object, "result", json_object_new_string(params));
	json_object_object_add(my_object, "id", json_object_new_int(id));
	printf("my response:%s\n", json_object_to_json_string(my_object));
	json_object_put(my_object);
*/
	struct json_object *new_obj;

	if((method_no<1)||(method_no>40)){
		printf("method error!\n");
		return 1;
	}

	if(test_result==0){
		new_obj = json_tokener_parse(dist[method_no*2-2]);
	}else if(test_result==1){
		new_obj = json_tokener_parse(dist[method_no*2-1]);
	}else{
		printf("package():test_result error!\n");
	}
	if (!new_obj){
		printf("package():json_tokener_parse error!\n");
		return 0; 
	}
//	printf("json_object=%s\n",json_object_to_json_string(new_obj));
	strcpy(buf, json_object_to_json_string(new_obj));

	json_object_put(new_obj);

	return 0;
}


