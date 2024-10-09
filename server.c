#include "server.h"

Server svr;                // server
Request* requests = NULL;  // point to a list of requests
TrainInfo trains[TRAIN_NUM];
int maxfd;  // size of open file descriptor table, size of request list
int num_conn = 1;
int alive_conn = 0;

const unsigned char IAC_IP[3] = "\xff\xf4";
const char* file_prefix = "./csie_trains/train_";
const char* accept_read_header = "ACCEPT_FROM_READ";
const char* accept_write_header = "ACCEPT_FROM_WRITE";
const char* welcome_banner =
    "======================================\n"
    " Welcome to CSIE Train Booking System \n"
    "======================================\n";

const char* lock_msg = ">>> Locked.\n";
const char* exit_msg = ">>> Client exit.\n";
const char* cancel_msg = ">>> You cancel the seat.\n";
const char* full_msg = ">>> The shift is fully booked.\n";
const char* seat_booked_msg = ">>> The seat is booked.\n";
const char* no_seat_msg = ">>> No seat to pay.\n";
const char* book_succ_msg = ">>> Your train booking is successful.\n";
const char* invalid_op_msg = ">>> Invalid operation.\n";

#ifdef READ_SERVER
char* read_shift_msg = "Please select the shift you want to check [902001-902005]: ";
#elif defined WRITE_SERVER
char* write_shift_msg = "Please select the shift you want to book [902001-902005]: ";
char* write_seat_msg = "Select the seat [1-40] or type \"pay\" to confirm: ";
char* write_seat_or_exit_msg = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";
#endif

// initailize a server, exit for error
static void init_server(unsigned short port);

// initailize a request instance
static void init_request(Request* req);

// free resources used by a request instance
static void free_request(Request* req);

// accept connection
int accept_conn(void);

// get record filepath
static void getfilepath(char* filepath, int extension);

/*  Return value:
 *      1: read successfully
 *      0: read EOF (client down)
 *     -1: read failed
 *   TODO: handle incomplete input
 */
int handle_read(Request* req) {
    int r;
    char buf[MAX_MSG_LEN];
    size_t len;

    memset(buf, 0, sizeof(buf));

    // Read in request from client
    r = read(req->conn_fd, buf, sizeof(buf));
    if (r < 0) return -1;
    if (r == 0) return 0;
    char* p1 = strstr(buf, "\015\012");  // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\012");  // \n
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

    int conn_fd;  // fd for file that we open for reading
    char buf[MAX_MSG_LEN * 2], filename[FILE_LEN];

    int i, j;
    for (i = TRAIN_ID_START, j = 0; i <= TRAIN_ID_END; i++, j++) {
        getfilepath(filename, i);
#ifdef READ_SERVER
        trains[j].file_fd = open(filename, O_RDONLY);
#elif defined WRITE_SERVER
        trains[j].file_fd = open(filename, O_RDWR);
#else
        trains[j].file_fd = -1;
#endif
        if (trains[j].file_fd < 0) {
            ERR_EXIT("open");
        }
    }

    // Initialize server
    init_server((unsigned short)atoi(argv[1]));
    fprintf(stderr, "\nstarting on %.80s, port %d, fd %d, maxconn %d...\n", svr.hostname, svr.port, svr.listen_fd,
            maxfd);

    // Loop for handling connections
    while (1) {
        // TODO: Add IO multiplexing

        // Check new connection
        conn_fd = accept_conn();
        if (conn_fd < 0) continue;

        int ret = handle_read(&requests[conn_fd]);
        if (ret < 0) {
            fprintf(stderr, "bad request from %s\n", requests[conn_fd].host);
            continue;
        }

        // TODO: handle requests from clients
#ifdef READ_SERVER
        sprintf(buf, "%s : %s", accept_read_header, requestP[conn_fd].buf);
        write(requestP[conn_fd].conn_fd, buf, strlen(buf));
#elif defined WRITE_SERVER
        sprintf(buf, "%s : %s", accept_write_header, requestP[conn_fd].buf);
        write(requestP[conn_fd].conn_fd, buf, strlen(buf));
#endif

        close(requests[conn_fd].conn_fd);
        free_request(&requests[conn_fd]);
    }

    free(requests);
    close(svr.listen_fd);
    for (i = 0; i < TRAIN_NUM; i++) close(trains[i].file_fd);

    return 0;
}

int accept_conn(void) {
    struct sockaddr_in cliaddr;
    size_t clilen;
    int conn_fd;  // fd for a new connection with client

    clilen = sizeof(cliaddr);
    conn_fd = accept(svr.listen_fd, (struct sockaddr*)&cliaddr, (socklen_t*)&clilen);
    if (conn_fd < 0) {
        if (errno == EINTR || errno == EAGAIN) return -1;  // try again
        if (errno == ENFILE) {
            (void)fprintf(stderr, "out of file descriptor table ... (maxconn %d)\n", maxfd);
            return -1;
        }
        ERR_EXIT("accept");
    }

    requests[conn_fd].conn_fd = conn_fd;
    strcpy(requests[conn_fd].host, inet_ntoa(cliaddr.sin_addr));
    fprintf(stderr, "getting a new request... fd %d from %s\n", conn_fd, requests[conn_fd].host);
    requests[conn_fd].client_id = (svr.port * 1000) + num_conn;  // This should be unique for the same machine.
    num_conn++;

    return conn_fd;
}

static void getfilepath(char* filepath, int extension) {
    char fp[FILE_LEN * 2];

    memset(filepath, 0, FILE_LEN);
    sprintf(fp, "%s%d", file_prefix, extension);
    strcpy(filepath, fp);
}

// ======================================================================================================
// You don't need to know how the following codes are working
#include <fcntl.h>

static void init_request(Request* req) {
    req->conn_fd = -1;
    req->client_id = -1;
    req->buf_len = 0;
    req->status = INVALID;
    req->remaining_time.tv_sec = 5;
    req->remaining_time.tv_usec = 0;

    req->booking_info.num_chosen_seats = 0;
    req->booking_info.train_fd = -1;
    for (int i = 0; i < SEAT_NUM; i++) req->booking_info.seat_stat[i] = UNKNOWN;
}

static void free_request(Request* req) {
    memset(req, 0, sizeof(Request));
    init_request(req);
}

static void init_server(unsigned short port) {
    struct sockaddr_in servaddr;
    int tmp;

    gethostname(svr.hostname, sizeof(svr.hostname));
    svr.port = port;

    svr.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (svr.listen_fd < 0) ERR_EXIT("socket");

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    tmp = 1;
    if (setsockopt(svr.listen_fd, SOL_SOCKET, SO_REUSEADDR, (void*)&tmp, sizeof(tmp)) < 0) {
        ERR_EXIT("setsockopt");
    }
    if (bind(svr.listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        ERR_EXIT("bind");
    }
    if (listen(svr.listen_fd, 1024) < 0) {
        ERR_EXIT("listen");
    }

    // Get file descripter table size and initialize request table
    maxfd = getdtablesize();
    requests = (Request*)malloc(sizeof(Request) * maxfd);
    if (requests == NULL) {
        ERR_EXIT("out of memory allocating all requests");
    }
    for (int i = 0; i < maxfd; i++) {
        init_request(&requests[i]);
    }
    requests[svr.listen_fd].conn_fd = svr.listen_fd;
    strcpy(requests[svr.listen_fd].host, svr.hostname);
}
