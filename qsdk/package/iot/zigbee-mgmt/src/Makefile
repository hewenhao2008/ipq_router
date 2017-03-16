
all: zigbee-mgmt zigbee-mgmt-ut 

APPS_SRC=zigbee_mgmt_main.c serial.c zb_serial_protocol.c zb_session_mgmt.c dev_mgmt.c zb_serial_rx_thread.c zb_cmds.c \
					 biz_msg_parser.c biz_msg_process.c dev_search_thread.c  zb_data_type.c

#for unit test only.
UINT_SRC=unit_test_main.c serial.c zb_serial_protocol.c zb_session_mgmt.c dev_mgmt.c zb_serial_rx_thread.c  zb_cmds.c biz_msg_parser.c \
					biz_msg_process.c dev_search_thread.c zb_data_type.c

zigbee-mgmt :
	$(CC) $(CFLAGS) -o zigbee-mgmt $(APPS_SRC)	$(LDFLAGS)
		

zigbee-mgmt-ut :
	$(CC) $(CFLAGS) -o zigbee-mgmt-ut $(UINT_SRC)	$(LDFLAGS)
	
clean:
	rm -f zigbee-mgmt zigbee-mgmt-ut
