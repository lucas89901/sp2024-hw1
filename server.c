#include "server.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

#if defined(DEBUG) && !defined(DEBUG_TIMEOUT)
#define CONNECTION_TIMEOUT 100000.0
#else
#define CONNECTION_TIMEOUT 5.0
#endif

static const char* FILE_PREFIX = "./csie_trains/train_";

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

    int flag = fcntl(svr->listen_fd, F_GETFL);
    if (flag < 0) {
        ERR_EXIT("init_server fcntl F_GETFL");
    }
    if (fcntl(svr->listen_fd, F_SETFL, flag | O_NONBLOCK) < 0) {
        ERR_EXIT("init_server fcntl F_SETFL");
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

    init_request(&requests[svr->listen_fd]);
    requests[svr->listen_fd].conn_fd = svr->listen_fd;
    strcpy(requests[svr->listen_fd].host, svr->hostname);
    svr->num_conn = 1;

    fprintf(stderr, "Starting server on %.80s, port %d, fd %d...\n", svr->hostname, svr->port, svr->listen_fd);
}

// Accepts a new connection.
// Returns the file descriptor used to communicate with the connection, or -1 if an error has occurred.
static int accept_conn(Server* svr, Request* requests, struct pollfd* pollfds, int* pollfds_size, const int maxfd) {
    struct sockaddr_in cliaddr;
    size_t clilen = sizeof(cliaddr);

    // fd for a new connection with client
    int conn_fd = accept(svr->listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) return -1;  // try again
        if (errno == ENFILE) {
            LOG("out of file descriptor table ... (maxconn %d)", getdtablesize());
            return -1;
        }
        ERR_EXIT("accept");
    }

    init_request(&requests[conn_fd]);
    requests[conn_fd].conn_fd = conn_fd;
    strcpy(requests[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
    LOG("Accept request, fd %d from %s", conn_fd, requests[conn_fd].host);
    requests[conn_fd].client_id = (svr->port * 1000) + svr->num_conn;  // This should be unique for the same machine.
    ++svr->num_conn;

    if (clock_gettime(CLOCK_MONOTONIC, &requests[conn_fd].receive_time) < 0) {
        ERR_EXIT("clock_gettime");
    }
    LOG("tv_sec=%ld, tv_nsec=%ld", requests[conn_fd].receive_time.tv_sec, requests[conn_fd].receive_time.tv_nsec);

    // Add new connection to `pollfds`.
    int pollfd_target = *pollfds_size;
    for (int i = 0; i < *pollfds_size; ++i) {
        if (pollfds[i].events == 0) {
            pollfd_target = i;
            break;
        }
    }
    pollfds[pollfd_target].fd = conn_fd;
    pollfds[pollfd_target].events = POLLIN;
    if (pollfd_target == *pollfds_size) {
        if (*pollfds_size == maxfd) {
            ERR_EXIT("out of fds");
        }
        ++(*pollfds_size);
    }

    write_message(&requests[conn_fd], kWelcomeBanner);
    write_prompt(&requests[conn_fd]);
    return conn_fd;
}

void close_conn(Request* req, struct pollfd* pollfd) {
    LOG("Close connection, fd=%d", pollfd->fd);
    cleanup_request(req);
    close(pollfd->fd);
    pollfd->fd = -1;
    pollfd->events = 0;
}

int main(int argc, char** argv) {
    // Parse args.
    if (argc != 2) {
        fprintf(stderr, "usage: %s [port]\n", argv[0]);
        exit(1);
    }

    Shift* shifts = malloc(sizeof(Shift) * SHIFT_NUM);
    for (int i = 0; i < SHIFT_NUM; i++) {
        shifts[i].id = i;

        char path[FILE_LEN];
        memset(path, 0, FILE_LEN);
        sprintf(path, "%s%d", FILE_PREFIX, SHIFT_ID_START + i);
#ifdef READ_SERVER
        shifts[i].fd = open(path, O_RDONLY);
#elif defined WRITE_SERVER
        shifts[i].fd = open(path, O_RDWR);
#endif
        if (shifts[i].fd < 0) {
            ERR_EXIT("open");
        }

        for (int j = 1; j <= SEAT_NUM; ++j) {
            shifts[i].reserver[j] = -1;
            shifts[i].payer[j] = -1;
        }
    }

    // size of open file descriptor table, size of request list
    const int maxfd = getdtablesize();
    Request* requests = (Request*)malloc(sizeof(Request) * maxfd);
    if (requests == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    fprintf(stderr, "Initalized request array, maxconn=%d\n", maxfd);

    // Initialize server
    Server svr;
    init_server(&svr, (unsigned short)atoi(argv[1]), requests);

    struct pollfd* pollfds = malloc(sizeof(struct pollfd) * maxfd);
    int pollfds_size = 0;

    // Loop for handling connections
    while (1) {
        // Close connections that have timed-out.
        for (int i = 0; i < pollfds_size; ++i) {
            if (pollfds[i].fd < 0) {
                continue;
            }
            Request* req = &requests[pollfds[i].fd];
            struct timespec now;
            if (clock_gettime(CLOCK_MONOTONIC, &now) < 0) {
                ERR_EXIT("clock_gettime");
            }
            if ((double)(now.tv_sec - req->receive_time.tv_sec) + (now.tv_nsec - req->receive_time.tv_nsec) / 1e9 >=
                CONNECTION_TIMEOUT) {
                LOG("connection %d has timed out", pollfds[i].fd);
                close_conn(req, &pollfds[i]);
            }
        }

        // Check for new connections.
        int conn_fd = accept_conn(&svr, requests, pollfds, &pollfds_size, maxfd);

        // Poll for ready connections.
        int ready = poll(pollfds, pollfds_size, 10);
        if (ready < 0) {
            ERR_EXIT("poll");
        }
        if (ready == 0) {
            continue;
        }
        for (int i = 0; i < pollfds_size; ++i) {
            if (!(pollfds[i].revents & POLLIN)) {
                continue;
            }

            Request* req = &requests[pollfds[i].fd];
            bool cleanup = false;
            int ret = read_connection(req, shifts);
            LOG("read_connection ret=%d", ret);
            if (ret <= 0) {
                close_conn(req, &pollfds[i]);
            }
        }
    }

    close(svr.listen_fd);
    for (int i = 0; i < SHIFT_NUM; i++) {
        close(shifts[i].fd);
    }
    free(shifts);
    free(requests);

    return 0;
}
