SRCS:=$(wildcard *.c)
OBJS:=$(SRCS:.c=.o)
CFLAGS+=-Wall
LDLIBS:=-lubus -lubox -lblobmsg_json -ljson-c

BIN:=vpn_manager

.PHONY:all clean

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $^ 

clean: 
	$(RM) $(BIN) $(OBJS)