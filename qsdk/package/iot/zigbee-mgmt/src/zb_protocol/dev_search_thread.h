#ifndef DEV_SEARCH_THREAD_H
#define DEV_SEARCH_THREAD_H

#include "biz_msg_process.h"
#include "zb_serial_protocol.h"


int init_dev_search_thread(void);

int start_dev_search_thread(void);

//void add_node_to_search_list(biz_node_t * pnode);

void add_node_to_search_list_new(struct serial_session_node * pnode);


#endif

