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

static const unsigned char IAC_IP[3] = "\xff\xf4";
static const char* FILE_PREFIX = "./csie_trains/train_";
static const char* ACCEPT_READ_HEADER = "ACCEPT_FROM_READ";
static const char* ACCEPT_WRITE_HEADER = "ACCEPT_FROM_WRITE";
static const char* WELCOME_BANNER =
    "======================================\n"
    " Welcome to CSIE Train Booking System \n"
    "======================================\n";

static const char* LOCK_MSG = ">>> Locked.\n";
static const char* EXIT_MSG = ">>> Client exit.\n";
static const char* CANCEL_MSG = ">>> You cancel the seat.\n";
static const char* FULL_MSG = ">>> The shift is fully booked.\n";
static const char* SEAT_BOOKED_MSG = ">>> The seat is booked.\n";
static const char* NO_SEAT_MSG = ">>> No seat to pay.\n";
static const char* BOOK_SUCC_MSG = ">>> Your train booking is successful.\n";
static const char* INVALID_OP_MSG = ">>> Invalid operation.\n";

#ifdef READ_SERVER
static char* READ_SHIFT_MSG = "Please select the shift you want to check [902001-902005]: ";
#elif defined WRITE_SERVER
static char* WRITE_SHIFT_MSG = "Please select the shift you want to book [902001-902005]: ";
static char* WRITE_SEAT_MSG = "Select the seat [1-40] or type \"pay\" to confirm: ";
static char* WRITE_SEAT_OR_EXIT_MSG = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";
#endif

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

// accept connection
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

/*  Return value:
 *      1: read successfully
 *      0: read EOF (client down)
 *     -1: read failed
 *   TODO: handle incomplete input
 */
int handle_read(Request* req) {
    char buf[MAX_MSG_LEN];
    memset(buf, 0, sizeof(buf));
    size_t len;

    // Read in request from client
    int r = read(req->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\r\n");  // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\n");  // \n
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client presses ctrl+C....\n");
                return 0;
            }
        }
    }

    len = p1 - buf + 1;
    memmove(req->buf, buf, len);
    req->buf[len - 1] = '\0';
    req->buf_len = len - 1;
    return 1;
}

#ifdef READ_SERVER
int print_train_info(Request* req) {
    int i;
    char buf[MAX_MSG_LEN];

    memset(buf, 0, sizeof(buf));
    for (i = 0; i < SEAT_NUM / 4; i++) {
        sprintf(buf + (i * 4 * 2), "%d %d %d %d\n", 0, 0, 0, 0);
    }
    return 0;
}
#else
int print_train_info(Request* req) {
    /*
     * Booking info
     * |- Shift ID: 902001
     * |- Chose seat(s): 1,2
     * |- Paid: 3,4
     */
    char buf[MAX_MSG_LEN * 3];
    char chosen_seat[MAX_MSG_LEN] = "1,2";
    char paid[MAX_MSG_LEN] = "3,4";

    memset(buf, 0, sizeof(buf));
    sprintf(buf,
            "\nBooking info\n"
            "|- Shift ID: %d\n"
            "|- Chose seat(s): %s\n"
            "|- Paid: %s\n\n",
            902001, chosen_seat, paid);
    return 0;
}
#endif

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

    char buf[MAX_MSG_LEN * 2];
    // Loop for handling connections
    while (1) {
        // TODO: Add IO multiplexing

        // Check new connection
        int conn_fd = accept_conn(&svr, requests, &num_conn);
        if (conn_fd < 0) {
            continue;
        }

        int ret = handle_read(&requests[conn_fd]);
        if (ret < 0) {
            fprintf(stderr, "bad request from %s\n", requests[conn_fd].host);
            continue;
        }

        // TODO: handle requests from clients
#ifdef READ_SERVER
        sprintf(buf, "%s : %s", ACCEPT_READ_HEADER, requests[conn_fd].buf);
        write(conn_fd, buf, strlen(buf));
#elif defined WRITE_SERVER
        sprintf(buf, "%s : %s", ACCEPT_WRITE_HEADER, requests[conn_fd].buf);
        write(conn_fd, buf, strlen(buf));
#endif

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
