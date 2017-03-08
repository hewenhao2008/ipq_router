#include "zb_session_mgmt.h"

#include "zb_serial_protocol.h"

char cmd[5] = {0x11,0x22,0x33,0x44,0x55};
char payload[3] = {0x38,0x38,0x38};
int payload_len = 3;

void test(void)
{
	struct ser_addr app;
	struct ser_addr zb_end;
	struct serial_session_node *pSession;
	int ret;

	app.addr[0] = 0x00;
	app.addr[1] = 0x00;
	app.addr[2] = 0x11;

	zb_end.addr[0] = 0x00;
	zb_end.addr[1] = 0x11;
	zb_end.addr[2] = 0x22;

	printf("--- add serial session.\n");
	pSession = find_or_add_serial_session(&app, &zb_end);
	if( pSession == NULL)
	{
		printf(">>> Failed to create session.\n");
		return;
	}

	ret = zb_protocol_send(pSession, cmd, payload, payload_len);
	if( ret  == 0)
	{
		printf(">> send success.\n");
	}
	else
	{
		printf(">> send fail.\n");
	}

	//kill tx thread.

	sleep(60);

	printf("------ test end -----\n");
		
}

