# build link_status and link_type executable when user executes "make"

all:bin/link_type_detect_core bin/link_status_detect_core
.PHONY : all

bin/link_status_detect_core: link_status.o
	$(CC) $(LDFLAGS) link_status.o -o $@

bin/link_type_detect_core: wan_type.o
	$(CC) $(LDFLAGS) wan_type.o -o $@

%.o: %.c
	@echo Compile $<
	@$(CC) $(CFLAGS) -c $< -o $@ 

# remove object files and executable when user executes "make clean"
clean:
	rm *.o bin/*