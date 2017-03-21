
EVENT_LOOP_LDFLAGS=$(LDFLAGS) -liot_util -ljson-c

all: iot_event_loop
	

iot_event_loop: iot_event_loop.c iot_msg_deal.c parse_json.c
	$(CC) $(CFLAGS) $(EVENT_LOOP_LDFLAGS) -o $@ \
		        iot_event_loop.c iot_msg_deal.c parse_json.c


clean:
	-rm -f *.o *.elf *.gbd



