#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "request.h"
#include "shift.h"

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

int shift_str_to_id(const char* const s, const int len) {
    if (len != 6) {
        return -1;
    }

    errno = 0;
    char* end;
    int shift = strtol(s, &end, 10);
    if (errno != 0 || end != s + len) {
        return -1;
    }

    shift -= SHIFT_ID_START;
    if (shift < 0 || shift >= SHIFT_NUM) {
        return -1;
    }
    return shift;
}

int seat_str_to_int(const char* const s, const int len) {
    if (len > 2) {
        return -1;
    }

    errno = 0;
    char* end;
    int seat = strtol(s, &end, 10);
    if (errno != 0 || end != s + len) {
        return -1;
    }

    if (seat < 1 || seat > SEAT_NUM) {
        return -1;
    }
    return seat;
}

void join_seats(char* buf, const int* const holders, const Request* const req) {
    bool first = true;
    int pos = 0;
    for (int i = 1; i <= SEAT_NUM; ++i) {
        if (holders[i] == req->client_id) {
            if (first) {
                first = false;
            } else {
                pos += sprintf(buf + pos, ",");
            }
            pos += sprintf(buf + pos, "%d", i);
        }
    }
    buf[pos] = '\0';
}

int read_connection(Request* const req, Shift* const shifts) {
    // Read data from connection.
    int ret = read(req->conn_fd, req->buf + req->buf_len, REQUEST_BUFFER_LEN - req->buf_len);
    if (ret < 0) {
        return -1;
    }
    if (ret == 0) {
        return 0;
    }
    req->buf_len += ret;
    DEBUG("req->buf=%s,req->buf_len=%ld", req->buf, req->buf_len);
    req->buf[req->buf_len] = '\0';  // Mark current end of buffer.

    // Handle available commands.
    while (1) {
        if (req->buf + req->buf_len - req->cmd > 2 && strncmp(req->cmd, IAC_IP, 2) == 0) {
            // Client presses ctrl+C, regard as disconnection
            DEBUG("Client pressed Ctrl+C....");
            return 0;
        }

        // Check if buffer contains a complete command.
        char* eol = strchr(req->cmd, '\n');
        if (eol == NULL) {
            if (eol - req->cmd > MAX_MSG_LEN) {
                return -1;  // Command too long.
            }
            return 2;  // Partial command.
        }
        char* next_cmd = eol + 1;
        if (eol > req->buf && *(eol - 1) == '\r') {
            --eol;
        }
        *eol = '\0';
        req->cmd_len = eol - req->cmd;
        DEBUG("req->cmd=%s,req->cmd_len=%ld", req->cmd, req->cmd_len);

        if (strncmp(req->cmd, "exit", 4) == 0) {
            WRITE(req->conn_fd, EXIT_MSG, 18);
            return 0;
        }
        int ret = handle_command(req, shifts);
        DEBUG("handle_command ret=%d", ret);
        switch (ret) {
            case -1:
                WRITE(req->conn_fd, INVALID_OP_MSG, 24);
                return 0;
            case 0:
                write_prompt(req);
                break;
        }

        req->cmd = next_cmd;
        req->cmd_len = 0;
        return 1;
    }
}
