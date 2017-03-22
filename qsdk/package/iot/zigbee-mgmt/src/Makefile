
all: zigbee-mgmt zigbee-mgmt-ut zigbee-cli

PHY_SRC= phy_serial/serial_dev.c phy_serial/rdt_rx_thread.c phy_serial/rdt_tx_thread.c
BIZ_SRC= biz/biz_msg_in.c biz/biz_msg_parser.c biz/biz_msg_process.c zb_device/zb_db.c
PROT_SRC= zb_protocol/zb_serial_protocol.c zb_protocol/dev_search_thread.c zb_protocol/zb_cmds.c zb_protocol/zb_cmds_join.c zb_protocol/zb_cmds_light.c  zb_protocol/zb_data_type.c 
CLI_SRC= cli/cli_client_main.c cli/cli_client_readline.c cli/cli_cmds.c cli/cli_common.c
CLI_SER_SRC= cli/cli_server.c cli/cli_common.c

APPS_SRC= $(PHY_SRC) $(BIZ_SRC) $(PROT_SRC) zigbee_mgmt_main.c  dev_mgmt.c
					    

INC_FLAGS=-I./phy_serial/ -I./biz -I./zb_protocol -I./zb_device -I./cli 

#for unit test only.
UINT_SRC= $(PHY_SRC) $(BIZ_SRC) $(PROT_SRC) unit_test_main.c dev_mgmt.c 

zigbee-mgmt :
	$(CC) $(CFLAGS) $(INC_FLAGS) -o zigbee-mgmt $(APPS_SRC)	$(CLI_SER_SRC) $(LDFLAGS)
		

zigbee-mgmt-ut :
	$(CC) $(CFLAGS) $(INC_FLAGS) -o zigbee-mgmt-ut $(UINT_SRC) $(CLI_SER_SRC)	$(LDFLAGS)

zigbee-cli:
	$(CC) $(CFLAGS) $(INC_FLAGS) -o zigbee-cli $(CLI_SRC) $(LDFLAGS)
	
clean:
	rm -rf *.o
	
