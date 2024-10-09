all: read_server write_server

.PHONY: all clean

read_server: server.c server.h
	$(CC) -D READ_SERVER -o read_server server.c
write_server: server.c server.h
	$(CC) -D WRITE_SERVER -o write_server server.c

clean:
	rm -f read_server write_server
