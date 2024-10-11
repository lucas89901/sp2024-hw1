#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "request.h"
#include "shift.h"

static const unsigned char IAC_IP[3] = "\xff\xf4";

static const char* WELCOME_BANNER =
    "======================================\n"
    " Welcome to CSIE Train Booking System \n"
    "======================================\n";

static const char* const INVALID_OP_MSG = ">>> Invalid operation.\n";
static const char* const EXIT_MSG = ">>> Client exit.\n";

static const char* const FULL_MSG = ">>> The shift is fully booked.\n";
static const char* const LOCK_MSG = ">>> Locked.\n";
static const char* const BOOKED_MSG = ">>> The seat is booked.\n";
static const char* const CANCEL_MSG = ">>> You cancel the seat.\n";
static const char* const NO_SEAT_MSG = ">>> No seat to pay.\n";
static const char* const BOOK_SUCC_MSG = ">>> Your train booking is successful.\n";

static const char* const READ_SHIFT_MSG = "Please select the shift you want to check [902001-902005]: ";
static const char* const WRITE_SHIFT_MSG = "Please select the shift you want to book [902001-902005]: ";
static const char* const WRITE_SEAT_MSG = "Select the seat [1-40] or type \"pay\" to confirm: ";
static const char* const WRITE_SEAT_OR_EXIT_MSG = "Type \"seat\" to continue or \"exit\" to quit [seat/exit]: ";

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

int seats_joined_str(char* buf, const int* const holders, const Request* const req) {
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
    return pos;
}

void write_message(const Request* const req, Message message) {
    switch (message) {
        case kWelcomeBanner:
            WRITE(req->conn_fd, WELCOME_BANNER, 117);
            break;
        case kInvalidOp:
            WRITE(req->conn_fd, INVALID_OP_MSG, 23);
            break;
        case kExit:
            WRITE(req->conn_fd, EXIT_MSG, 17);
            break;
        case kShiftFull:
            WRITE(req->conn_fd, FULL_MSG, 31);
            break;
        case kSeatLocked:
            WRITE(req->conn_fd, LOCK_MSG, 12);
            break;
        case kSeatBooked:
            WRITE(req->conn_fd, BOOKED_MSG, 24);
            break;
        case kSeatCancel:
            WRITE(req->conn_fd, CANCEL_MSG, 25);
            break;
        case kPaymentSuccess:
            WRITE(req->conn_fd, BOOK_SUCC_MSG, 38);
            break;
        case kPaymentNoSeat:
            WRITE(req->conn_fd, NO_SEAT_MSG, 20);
            break;
        case kReadShiftPrompt:
            WRITE(req->conn_fd, READ_SHIFT_MSG, 59);
            break;
        case kWriteShiftPrompt:
            WRITE(req->conn_fd, WRITE_SHIFT_MSG, 58);
            break;
        case kWriteSeatPrompt:
            WRITE(req->conn_fd, WRITE_SEAT_MSG, 49);
            break;
        case kWritePostPaymentPrompt:
            WRITE(req->conn_fd, WRITE_SEAT_OR_EXIT_MSG, 55);
            break;
    }
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
    LOG("req->buf=%s,req->buf_len=%ld", req->buf, req->buf_len);
    req->buf[req->buf_len] = '\0';  // Mark current end of buffer.

    // Handle available commands.
    while (1) {
        if (req->buf + req->buf_len - req->cmd > 2 && strncmp(req->cmd, IAC_IP, 2) == 0) {
            // Client presses ctrl+C, regard as disconnection
            LOG("Client pressed Ctrl+C....");
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
        LOG("req->cmd=%s,req->cmd_len=%ld", req->cmd, req->cmd_len);

        if (strncmp(req->cmd, "exit", 4) == 0) {
            write_message(req, kExit);
            return 0;
        }
        int ret = handle_command(req, shifts);
        LOG("handle_command ret=%d", ret);
        switch (ret) {
            case -1:
                write_message(req, kInvalidOp);
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
