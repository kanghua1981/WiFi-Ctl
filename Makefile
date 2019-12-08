


CFLAGS += -O2 -DLINUX -D_GNU_SOURCE -Wall -DDEBUG 
SOURCE = $(wildcard ./*.c) $(wildcard ./qsdk/*.c)
OBJS = $(SOURCE:.c=.o)
INCDIR = ./ 
APP_BINARY = connectmng

all: $(APP_BINARY)

$(APP_BINARY): $(OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -lpthread -o $@
	
$(OBJS): %.o : %.c
	$(CC) $(CFLAGS) -O2 -I$(INCDIR) -c $< -o $@ 

clean:
	rm -rf *.o
	rm -rf $(APP_BINARY)

romfs:
	$(ROMFSINST) /bin/$(APP_BINARY)
#	$(ROMFSINST) gcom /etc_ro/gcom




