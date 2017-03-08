
all: zigbee-mgmt

APPS_SRC=zigbee_mgmt_main.c serial.c zb_serial_protocol.c zb_session_mgmt.c dev_mgmt.c

zigbee-mgmt :
	$(CC) $(CFLAGS) -o zigbee-mgmt $(APPS_SRC)	$(LDFLAGS)

	