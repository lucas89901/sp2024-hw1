#ifndef REQUEST_H_
#define REQUEST_H_

#include <stdbool.h>
#include <sys/time.h>
#include <sys/types.h>

#include "common.h"

typedef struct Shift Shift;

typedef struct Request {
    char host[512];  // client's host
    int conn_fd;     // fd to talk with client
    int client_id;   // client's id

    char buf[MAX_MSG_LEN];  // data sent by/to client
    size_t buf_len;         // bytes used by buf

    Shift* shift;
    int reserved_seat_num;

    struct timeval remaining_time;  // connection remaining time
} Request;

// initailize a request instance
void init_request(Request* req);

void cleanup_request(Request* req);

#endif  // REQUEST_H_
