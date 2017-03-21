#ifndef _IOT_MSG_UTIL_H_
#define _IOT_MSG_UTIL_H_

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/ipc.h>


typedef enum
{
  IOT_MSG_SQUEUE_EVENT_LOOP = 0x00,
  IOT_MSG_SQUEUE_DATABASE   = 0x01,
  IOT_MSG_SQUEUE_DEBUG      = 0x02,

}IOT_MSG_SQUEUE_NO;

typedef enum
{
	IOT_MODULE_NONE       = 0x00,
	IOT_MODULE_EVENT_LOOP = 0x01,
	IOT_MODULE_MQTT       = 0x02,
	IOT_MODULE_DATABASE   = 0x03,
	IOT_MODULE_WIFI       = 0x04,
  IOT_MODULE_ZIGBEE     = 0x05,
  IOT_MODULE_DEBUG      = 0x06,
}IOT_MODULE_ID;

#define IOT_MSG_BODY_LEN_MAX  8000

typedef struct
{
	long dst_mod;
  unsigned char src_mod;
  unsigned char handler_mod;
  unsigned int body_len;
  unsigned char body[IOT_MSG_BODY_LEN_MAX];
}IOT_MSG;


/*
  get or create a msg queue

  [in]queue_no: queue number to get
  [out]return:     failed == -1; success != -1
*/
int iot_msg_queue_get(IOT_MSG_SQUEUE_NO queue_no);

/*
  put msg to a msg queue

  [in]queue_id:         iot_msg_queue_get return value. (!=-1)
  [in]msg->dst_mod:     the module id to send
  [in]msg->src_mod:     msg creator's module id
  [in]msg->handler_mod: the module need to deal the msg(can set to none)
  [in]msg->body_len:the msg body's length
  [in]msg->body:    the msg body's context
  [out]return:          failed == -1; success == 0
*/
int iot_msg_send(int queue_id, IOT_MSG * msg);

/*
  get msg from a msg squeue, if none, waiting

  [in]queue_id:          iot_msg_queue_get return value. (!=-1)
  [in]msg->dst_mod:      the module id to send.(aller's module id)
  [out]msg->src_mod:     msg creator's module id
  [out]msg->handler_mod: the module need to deal the msg(can set to none)
  [out]msg->body_len:    the msg body's length
  [out]msg->body:        the msg body's context
  [out]return:           failed < 0; success == msg len 
*/
int iot_msg_recv_wait(int queue_id, IOT_MSG * msg_buf);

/*
  get msg from a msg squeue, if none, return 

  [in]queue_id:          iot_msg_queue_get return value. (!=-1)
  [in]msg->dst_mod:      the module id to send.(aller's module id)
  [out]msg->src_mod:     msg sender's module id
  [out]msg->handler_mod: the module need to deal the msg(can set to none)
  [out]msg->body_len:    the msg body's length
  [out]msg->body:        the msg body's context
  [out]return:           failed < 0; success == msg len or 0 
*/
int iot_msg_recv_nowait(int queue_id, IOT_MSG * msg_buf);


#endif
