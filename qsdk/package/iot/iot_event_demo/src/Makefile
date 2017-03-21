
MGR_LDFLAGS=$(LDFLAGS) -lpthread -liot_util 
					 

all: iot_event_demo
	

iot_event_demo: iot_event_demo.c mqtt_demo_thread.c
	$(CC) $(CFLAGS) $(MGR_LDFLAGS) -o $@ \
		iot_event_demo.c mqtt_demo_thread.c



clean:
	-rm -f *.o *.elf *.gbd



