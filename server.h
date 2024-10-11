#ifndef SERVER_H_
#define SERVER_H_

typedef struct Server {
    char hostname[512];   // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;        // fd to wait for a new connection
    int num_conn;
} Server;

#endif  // SERVER_H_
