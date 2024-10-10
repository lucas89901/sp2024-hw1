all: read_server write_server

.PHONY: all clean

io.o: io.c io.h common.h request.h
read.o: read.c read.h common.h shift.h io.h request.h
request.o: request.c request.h common.h shift.h
shift.o: shift.c shift.h common.h request.h
write.o: read.c read.h common.h shift.h io.h request.h

SERVER_OBJS = request.o io.o shift.o
SERVER_DEPS = server.c server.h common.h $(SERVER_OBJS)
read_server: $(SERVER_DEPS) read.o
	$(CC) -D READ_SERVER -o read_server server.c $(SERVER_OBJS) read.o
write_server: $(SERVER_DEPS) write.o
	$(CC) -D WRITE_SERVER -o write_server server.c $(SERVER_OBJS) write.o

clean:
	rm -f *.o read_server write_server
