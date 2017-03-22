#ifndef BIZ_MSG_IN_H
#define BIZ_MSG_IN_H

#include <iot_msg_util.h>

typedef int (* biz_msg_in_cb)(unsigned char *msg, unsigned int msg_len);


/*
  typedef struct
  {
	long dst_mod;
	unsigned char src_mod;
	unsigned char handler_mod;
	unsigned int body_len;
	unsigned char body[IOT_MSG_BODY_LEN_MAX];
  }IOT_MSG;

  get msg from a msg squeue, if none, waiting
  
  [in]queue_id:          iot_msg_queue_get return value. (!=-1)
  [in]msg->dst_mod:      the module id to send.(aller's module id)
  [out]msg->src_mod:     msg creator's module id
  [out]msg->handler_mod: the module need to deal the msg(can set to none)
  [out]msg->body_len:    the msg body's length
  [out]msg->body:        the msg body's context
  [out]return:           failed < 0; success == msg len 
*/


typedef struct biz_buff
{
	IOT_MSG	msg;
	
	char *data;	//point to msg->body.  IOT_MSG_BODY_LEN_MAX=8000
	
}biz_buff_t;


int init_biz_msg_queue(void);

int register_biz_msg_in_cb(biz_msg_in_cb cb);

int un_register_biz_msg_in_cb(biz_msg_in_cb cb);



#endif

