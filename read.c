#include <string.h>
#include <unistd.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

#define SHIFT_INFO_LEN 80

// Writes the seat map of a shift to the connection.
static void write_shift(const Request* const req) {
    char buf[SHIFT_INFO_LEN];
    memset(buf, 0, SHIFT_INFO_LEN);
    for (int i = 1; i <= SEAT_NUM; ++i) {
        char c;
        switch (seat_status(req, i)) {
            case kAvailable:
                c = '0';
                break;
            case kPaid:
                c = '1';
                break;
            case kReservedByOtherProcess:
            case kReservedByThisRequest:
            case kReservedByOtherRequest:
                c = '2';
                break;
        }
        sprintf(buf + seat_to_byte(i), "%c%c", c, " \n"[i % 4 == 0]);
    }
    WRITE(req->conn_fd, buf, SHIFT_INFO_LEN);
}

void write_prompt(const Request* const req) {
    switch (req->status) {
        case kShiftSelection:
            write_message(req, kReadShiftPrompt);
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
            write_shift(req);
            return 0;

        default:
            // Should not happen for a read server.
            return -1;
    }
}
