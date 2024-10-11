all: read_server write_server

.PHONY: all all_debug clean

all_debug: CFLAGS += -D DEBUG -fsanitize=address
all_debug: all

HDRS = common.h io.h request.h server.h shift.h
SERVER_SRCS = server.c
SERVER_DEPS = io.o request.o shift.o
$(SERVER_DEPS): $(HDRS)  # Rebuild everything if any header is modified

read_server: $(SERVER_SRCS) $(HDRS) $(SERVER_DEPS) read.o
	$(CC) $(CFLAGS) -D READ_SERVER -o read_server $(SERVER_SRCS) $(SERVER_DEPS) read.o
write_server: $(SERVER_SRCS) $(HDRS) $(SERVER_DEPS) write.o
	$(CC) $(CFLAGS) -D WRITE_SERVER -o write_server $(SERVER_SRCS) $(SERVER_DEPS) write.o

clean:
	rm -f *.o read_server write_server
