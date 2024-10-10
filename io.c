#include "io.h"

#include <stdio.h>
#include <string.h>

#include "request.h"

static const unsigned char IAC_IP[3] = "\xff\xf4";

const char* const INVALID_OP_MSG = ">>> Invalid operation.\n";
const char* const EXIT_MSG = ">>> Client exit.\n";

const char* const FULL_MSG = ">>> The shift is fully booked.\n";
const char* const LOCK_MSG = ">>> Locked.\n";
const char* const SEAT_BOOKED_MSG = ">>> The seat is booked.\n";
const char* const CANCEL_MSG = ">>> You cancel the seat.\n";
const char* const NO_SEAT_MSG = ">>> No seat to pay.\n";
const char* const BOOK_SUCC_MSG = ">>> Your train booking is successful.\n";

const char* const READ_SHIFT_MSG = "Please select the shift you want to check [902001-902005]: ";
const char* const WRITE_SHIFT_MSG = "Please select the shift you want to book [902001-902005]: ";
const char* const WRITE_SEAT_MSG = "Select the seat [1-40] or type \"pay\" to confirm: ";
const char* const WRITE_SEAT_OR_EXIT_MSG = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";

// Reads from the conenction, and moves the data read into `req->buf`.
// Returns 1 if data is successfully read, 0 if reached EOF (meaning client is down), and -1 if an error has occurred.
int handle_input(Request* req) {
    char buf[MAX_MSG_LEN + 1];
    memset(buf, 0, sizeof(buf));

    // Read in request from client.
    int ret = read(req->conn_fd, buf, sizeof(buf));
    if (ret < 0) {
        return -1;
    }
    if (ret == 0) {
        return 0;
    }

    // Look for line endings.
    char* p1 = strstr(buf, "\r\n");  // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\n");  // \n
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client pressed Ctrl+C....\n");
                return 0;
            }
        }
    }

    size_t len = p1 - buf + 1;
    memmove(req->buf, buf, len);
    req->buf[len - 1] = '\0';
    req->buf_len = len - 1;
    DEBUG("req->buf=%s,req->buf_len=%ld", req->buf, req->buf_len);

    if (strncmp(req->buf, "exit", 4) == 0) {
        if (write(req->conn_fd, EXIT_MSG, 18) < 0) {
            ERR_EXIT("write error");
        }
        return 0;
    }

    return 1;
}
