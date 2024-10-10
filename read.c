#include "read.h"

#include <string.h>
#include <unistd.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

int handle_read_request(Request* const req, const int* const shift_fds) {
    int ret;
    while (1) {
        WRITE(req->conn_fd, READ_SHIFT_MSG, 60);

        READ_COMMAND(req);
        req->shift_id = shift_str_to_id(req->buf, req->buf_len);
        if (req->shift_id < 0) {
            WRITE(req->conn_fd, INVALID_OP_MSG, 24);
            return -1;
        }
        req->shift_fd = shift_fds[req->shift_id];

        print_shift(req);
    }
}
