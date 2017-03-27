CFLAGS += -DOPENWRT

all_exe= client1 client2 client3 client4 server_test mock
all:$(all_exe)
#/usr/local/lib/libmosquitto.so -ljson-c -lpthread -lcrypto

LDFLAGS= -ljson-c -lpthread -lcrypto -lmosquitto -liot_util

files= client_shared.h client_shared.c parse.c msgque.c msgque.h

object1= client1.c $(files)
client1:$(object1)
	$(CC) $(CFLAGS) -o $@ $(object1)  $(LDFLAGS)
	
object2= client2.c $(files)
client2:$(object2)
	$(CC) $(CFLAGS) -o $@ $(object2) $(LDFLAGS) 

object3= client3.c $(files)
client3:$(object3)
	$(CC) $(CFLAGS) -o $@ $(object3) $(LDFLAGS) 

object4= client4.c $(files)
client4:$(object4)
	$(CC) $(CFLAGS) -o $@ $(object4) $(LDFLAGS) 

object5= server_test.c $(files)
server_test:$(object5)
	$(CC) $(CFLAGS) -o $@ $(object5) $(LDFLAGS) 

object6= mock.c $(files)
mock:$(object6)
	$(CC) $(CFLAGS) -o $@ $(object6) $(LDFLAGS) 

clean:
	-rm -f *.o *.elf *.gbd $(all_exe)
