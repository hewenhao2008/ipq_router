IOT_UTIL_LIB = libiot_util.so
IOT_UTIL_LIB_OBJ = iot_msg_util.o 


%.o: %.c
	$(CC) $(CFLAGS) -std=gnu99 -fstrict-aliasing -fPIC -c -o $@ $<


$(IOT_UTIL_LIB): clean $(IOT_UTIL_LIB_OBJ)  
	$(CC) $(LDFLAGS) -shared -o $@ $(IOT_UTIL_LIB_OBJ)



clean:
	rm -f *.o $(IOT_UTIL_LIB) $(IOT_UTIL_LIB_OBJ)
