#ifndef _PARSE_JSON_H_
#define _PARSE_JSON_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <json-c/json.h>

int get_method_uuid_from_json(char *json_str, char *method_buf, int method_buf_len,
								char *uuid_buf, int uuid_buf_len);

#endif