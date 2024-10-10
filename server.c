#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "request.h"

#ifdef READ_SERVER
#include "read.h"
#elif defined WRITE_SERVER
#include "write.h"
#else
#error "You must define one of READ_SERVER or WRITE_SERVER"
#endif

static const char* FILE_PREFIX = "./csie_trains/train_";
static const char* WELCOME_BANNER =
    "======================================\n"
    " Welcome to CSIE Train Booking System \n"
    "======================================\n";

// initailize a server, exit for error
static void init_server(Server* svr, unsigned short port, Request* requests) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr->hostname, sizeof(svr->hostname));
    svr->port = port;

    svr->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr->listen_fd < 0) {
        ERR_EXIT("socket");
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr->listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr->listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr->listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    requests[svr->listen_fd].conn_fd = svr->listen_fd;
    strcpy(requests[svr->listen_fd].host, svr->hostname);

    fprintf(stderr, "Starting server on %.80s, port %d, fd %d...\n", svr->hostname, svr->port, svr->listen_fd);
}

// Accepts a new connection.
// Returns the file descriptor used to communicate with the connection, or -1
// if an error has occurred.
static int accept_conn(Server* svr, Request* requests, int* num_conn) {
    struct sockaddr_in cliaddr;
    size_t clilen = sizeof(cliaddr);

    // fd for a new connection with client
    int conn_fd = accept(svr->listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) return -1;  // try again
        if (errno == ENFILE) {
            (void)fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", getdtablesize());
            return -1;
        }
        ERR_EXIT("accept");
    }

    requests[conn_fd].conn_fd = conn_fd;
    strcpy(requests[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requests[conn_fd].host);
    requests[conn_fd].client_id = (svr->port * 1000) + *num_conn;  // This should be unique for the same machine.
    (*num_conn)++;
    return conn_fd;
}

int main(int argc, char** argv) {
    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    // Open file descriptors for shift records.
    int shift_fds[SHIFT_NUM];
    for (int i = 0; i < SHIFT_NUM; i++) {
        char path[FILE_LEN];
        memset(path, 0, FILE_LEN);
        sprintf(path, "%s%d", FILE_PREFIX, SHIFT_ID_START + i);
#ifdef READ_SERVER
        shift_fds[i] = open(path, O_RDONLY);
#elif defined WRITE_SERVER
        shift_fds[i] = open(path, O_RDWR);
#else
        shift_fds[i] = -1;
#endif
        if (shift_fds[i] < 0) {
            ERR_EXIT("open");
        }
    }

    // size of open file descriptor table, size of request list
    const int maxfd = getdtablesize();
    Request* requests = (Request*)malloc(sizeof(Request) * maxfd);
    if (requests == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requests[i]);
    }
    fprintf(stderr, "Initalized request array, maxconn=%d\n", maxfd);

    // Initialize server
    Server svr;
    init_server(&svr, (unsigned short)atoi(argv[1]), requests);
    int num_conn = 1;

    // Loop for handling connections
    while (1) {
        // TODO: Add IO multiplexing

        // Check for new connections.
        int conn_fd = accept_conn(&svr, requests, &num_conn);
        if (conn_fd < 0) {
            continue;
        }

        write(conn_fd, WELCOME_BANNER, 118);
        int ret;
#ifdef READ_SERVER
        ret = handle_read_request(&requests[conn_fd], shift_fds);
#elif defined WRITE_SERVER
        ret = handle_write_request(&requests[conn_fd], shift_fds);
#endif
        DEBUG("handle_request=%d", ret);

        close(conn_fd);
        init_request(&requests[conn_fd]);
    }

    free(requests);
    close(svr.listen_fd);
    for (int i = 0; i < SHIFT_NUM; i++) {
        close(shift_fds[i]);
    }

    return 0;
}
