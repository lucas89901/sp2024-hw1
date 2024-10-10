#include "request.h"

#include <string.h>

#include "shift.h"

void init_request(Request* req) {
    memset(req, 0, sizeof(Request));

    req->conn_fd = -1;
    req->client_id = -1;
    req->buf_len = 0;
    req->status = INVALID;
    req->remaining_time.tv_sec = 5;
    req->remaining_time.tv_usec = 0;

    req->num_chosen_seats = 0;
    req->shift_id = -1;
    req->shift_fd = -1;
    for (int i = 0; i < SEAT_NUM; i++) {
        req->seats[i] = UNKNOWN;
    }
}

void cleanup_request(Request* req) {
    int ret;
    for (int i = 1; i <= SEAT_NUM; ++i) {
        if (req->seats[i] == CHOSEN) {
            ret = unlock_seat(req->shift_fd, i);
            req->seats[i] = AVAILABLE;
        }
    }
    DEBUG("ret=%d", ret);
}
