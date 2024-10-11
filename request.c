#include "request.h"

#include <string.h>

#include "shift.h"

void init_request(Request* req) {
    memset(req, 0, sizeof(Request));

    req->conn_fd = -1;
    req->client_id = -1;

    req->buf_len = 0;
    req->cmd = req->buf;
    req->cmd_len = 0;

    req->shift = NULL;
    req->reserved_seat_num = 0;

    req->status = kShiftSelection;
}

void cleanup_request(Request* req) {
    for (int i = 1; i <= SEAT_NUM; ++i) {
        if (req->shift && req->shift->reserver[i] == req->client_id) {
            cancel_seat(req, i);
        }
    }
    init_request(req);
}
