#ifndef ZB_DB_H
#define ZB_DB_H

#include "common_def.h"

int init_zigbee_db(void);
int db_get_endpoints(char *ManufactureName, char *ModelIdentifier, zb_uint8 endp_addr[]);


#endif

