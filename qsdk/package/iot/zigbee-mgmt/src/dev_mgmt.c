#include "dev_mgmt.h"

int zb_dev_add(zb_uint16 shortid, zb_uint8 *mac, zb_uint8 capability)
{
	int i=0;
	struct zigbee_dev *pzb_dev;
	
	ZB_PRINT(">> add device. short_id=0x%x  mac:", shortid);

	for(i=0; i<8; i++)
	{
		ZB_PRINT("%.2X-", mac[i]&0xFF);
	}

	
}

int zb_dev_get_endp_cnt(zb_uint16 shortid)
{
	return 1;
}

zb_uint8 zb_dev_get_endp(zb_uint16 shortid, int endp_index)
{
	return 0x0B;
}



zb_uint8 tmp_mac[8];
zb_uint8 * zb_dev_get_mac_by_short_id(zb_uint16 short_id)
{
	return tmp_mac;
}

int zb_dev_del()
{

}



