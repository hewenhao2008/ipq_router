#ifndef DEV_SEARCH_THREAD_H
#define DEV_SEARCH_THREAD_H

#include "biz_msg_process.h"

int init_dev_search_thread(void);

int start_dev_search_thread(void);

void add_node_to_search_list(biz_node_t * pnode);

#endif

