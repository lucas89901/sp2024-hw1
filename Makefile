all: read_server write_server

.PHONY: all clean

request.o: request.c common.h request.h

read_server: server.c server.h common.h request.o
	$(CC) -D READ_SERVER -o read_server server.c request.o
write_server: server.c server.h common.h request.o
	$(CC) -D WRITE_SERVER -o write_server server.c request.o

clean:
	rm -f read_server write_server
