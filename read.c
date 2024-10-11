#include <string.h>
#include <unistd.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

#define SHIFT_INFO_LEN 80

void write_prompt(const Request* const req) {
    switch (req->status) {
        case kShiftSelection:
            WRITE(req->conn_fd, READ_SHIFT_MSG, 60);
            return;

        default:
            // Should not happen for a read server.
            return;
    }
}

int handle_command(Request* const req, Shift* const shifts) {
    switch (req->status) {
        case kShiftSelection:;
            int shift_id = shift_str_to_id(req->cmd, req->cmd_len);
            if (shift_id < 0) {
                return -1;
            }
            req->shift = &shifts[shift_id];

            char buf[SHIFT_INFO_LEN];
            memset(buf, 0, SHIFT_INFO_LEN);
            print_shift(buf, req);
            WRITE(req->conn_fd, buf, SHIFT_INFO_LEN);
            return 0;

        default:
            // Should not happen for a read server.
            return -1;
    }
}
