
CFLAGS += -Ilib/include

DTLS_LIBS = -lgmp \
            -lhogweed \
            -lnettle \
            -lgnutls

APP_INC = -I./test/

PSRC = lib/src/coap_client.c lib/src/coap_log.c lib/src/coap_msg.c lib/src/coap_server.c
POBJ = $(PSRC:.c=.o)


all: libfreecoap.a libfreecoap.so freecoap test_coap_server test_coap_client

libfreecoap.a: $(POBJ)
	$(AR) rc $@ $(POBJ)
	
libfreecoap.so: $(POBJ)
	$(CC) -shared $(LDFLAGS) -o libfreecoap.so $(POBJ)

test_coap_server: test/test_coap_server/test_coap_server.c
	$(CC) $(CFLAGS) $(APP_INC) -o test_coap_server test/test_coap_server/test_coap_server.c test/test.c -L./ $(LDFLAGS) $(DTLS_LIBS) -lfreecoap 

test_coap_client: test/test_coap_client/test_coap_client.c
	$(CC) $(CFLAGS) $(APP_INC) -o test_coap_client test/test_coap_client/test_coap_client.c test/test.c -L./ $(LDFLAGS) $(DTLS_LIBS) -lfreecoap

freecoap: libfreecoap.so
	echo "==============build FreeCoap=========="
	touch freecoap

clean:
	rm libfreecoap.a libfreecoap.so lib/src/*.o 
