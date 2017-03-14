#include "parse_json.h"

static int get_json_string_member(struct json_object *new_obj, const char *field,
									char *method_buf, int buf_len);

int get_method_uuid_from_json(char *json_str, char *method_buf, int method_buf_len,
								char *uuid_buf, int uuid_buf_len)
{
	int res;
	struct json_object *new_obj;
	
	new_obj = json_tokener_parse(json_str);
	
	if (!new_obj){
		fprintf(stderr, "json tokener parse result is NULL\n");
		return -1; 
	}
	res = get_json_string_member(new_obj, "method", method_buf, method_buf_len);
	if(res < 0)
	{
		json_object_put(new_obj);
		return res;		
	}
	
	uuid_buf[0] = 0;
	struct json_object *params_obj = json_object_object_get(new_obj, "params");
	if(params_obj){
		if(json_object_is_type(params_obj, json_type_object))
		{
			get_json_string_member(params_obj, "device_uuid", uuid_buf, uuid_buf_len);
		}
		json_object_put(params_obj);
	}

	json_object_put(new_obj);
	return res;
}

static int get_json_string_member(struct json_object *new_obj, const char *field, 
									char *buf, int buf_len)
{
	struct json_object *obj = json_object_object_get(new_obj, field);
	if(!obj){
		fprintf(stderr, "obj get field %s obj failed\n", field);
		return -1;
	}

	if(!json_object_is_type(obj, json_type_string))
	{
		fprintf(stderr, "obj field %s is not string type\n", field);
		json_object_put(obj);
		return -1;
	}

	char *val = json_object_get_string(obj);
	if(!val){
		fprintf(stderr, "obj field %s get json string failed\n", field);
		json_object_put(obj);
		return -1;		
	}
	strncpy(buf, val, buf_len);
	json_object_put(obj);
	return 0;		
}