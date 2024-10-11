#ifndef REQUEST_H_
#define REQUEST_H_

#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

#include "common.h"

#define REQUEST_BUFFER_LEN (MAX_MSG_LEN * 10)

typedef struct Shift Shift;

typedef enum RequestStatus {
    kShiftSelection = 0,
    kSeatSelection,
    kPostPayment,
} RequestStatus;

typedef struct Request {
    char host[512];  // client's host
    int conn_fd;     // fd to talk with client
    int client_id;   // client's id

    char buf[REQUEST_BUFFER_LEN];  // data sent by/to client;
    size_t buf_len;                // bytes used by buf

    char* cmd;
    size_t cmd_len;

    RequestStatus status;  // request status

    Shift* shift;
    int reserved_seat_num;

    struct timespec receive_time;
} Request;

// initailize a request instance
void init_request(Request* req);

void cleanup_request(Request* req);

#endif  // REQUEST_H_
