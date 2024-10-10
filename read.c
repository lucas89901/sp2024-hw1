#include "read.h"

#include <string.h>
#include <unistd.h>

#include "common.h"
#include "input.h"
#include "request.h"
#include "shift.h"

static const char* const INVALID_OP_MSG = ">>> Invalid operation.\n";
static const char* const READ_SHIFT_MSG = "Please select the shift you want to check [902001-902005]: ";

int handle_read_request(Request* const req, const int* const shift_fds) {
    while (1) {
        write(req->conn_fd, READ_SHIFT_MSG, 60);
        int ret = handle_input(req);
        if (ret <= 0) {
            return ret;
        }

        if (!is_valid_shift(req->buf, req->buf_len)) {
            write(req->conn_fd, INVALID_OP_MSG, 24);
            return -1;
        }
        req->shift_id = req->buf[5] - '1';
        req->shift_fd = shift_fds[req->shift_id];

        print_shift(req);
    }
}
