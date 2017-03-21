#ifndef _MQTT_DEMO_THREAD_H_
#define _MQTT_DEMO_THREAD_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <pthread.h>

#include <iot_msg_util.h>

void mqtt_demo_thread_prepare();
void mqtt_demo_thread_exit();



#endif
