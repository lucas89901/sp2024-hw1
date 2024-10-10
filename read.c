#include "read.h"

#include <string.h>
#include <unistd.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

#define SHIFT_INFO_LEN 80

int handle_read_request(Request* const req, Shift* const shifts) {
    int ret;
    while (1) {
        WRITE(req->conn_fd, READ_SHIFT_MSG, 60);

        READ_COMMAND(req);
        int shift_id = shift_str_to_id(req->buf, req->buf_len);
        if (shift_id < 0) {
            WRITE(req->conn_fd, INVALID_OP_MSG, 24);
            return -1;
        }
        req->shift = &shifts[shift_id];

        char buf[SHIFT_INFO_LEN];
        memset(buf, 0, SHIFT_INFO_LEN);
        print_shift(buf, req);
        WRITE(req->conn_fd, buf, SHIFT_INFO_LEN);
    }
}
