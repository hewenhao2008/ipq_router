
//#include "public.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <json-c/json.h>


int parse(const char* str);
int getparams(void);
int package(const int method_no, const int test_result, char*buf);
