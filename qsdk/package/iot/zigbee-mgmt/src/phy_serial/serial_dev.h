#ifndef SERIAL_H
#define SERIAL_H

#include "common_def.h"

struct serial_dev
{
	int fd;
	
	fd_set rd_set;
	int fd_max;

	int stop;
	
};

typedef void (* serail_data_in_cb)(char *buf, int len);


int zb_init_serial(void);

#endif

