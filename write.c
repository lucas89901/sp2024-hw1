#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

// Writes booking info to the connection.
static void write_booking_info(const Request* const req) {
    char buf[1000];
    char reserved_seats[200];
    char paid_seats[200];
    memset(buf, 0, sizeof(buf));
    join_seats(reserved_seats, req->shift->reserver, req);
    join_seats(paid_seats, req->shift->payer, req);

    sprintf(buf,
            "\nBooking info\n"
            "|- Shift ID: %d\n"
            "|- Chose seat(s): %s\n"
            "|- Paid: %s\n\n",
            902001 + req->shift->id, reserved_seats, paid_seats);
    WRITE(req->conn_fd, buf, sizeof(buf));
}

void write_prompt(const Request* const req) {
    switch (req->status) {
        case kShiftSelection:
            WRITE(req->conn_fd, WRITE_SHIFT_MSG, 59);
            return;

        case kSeatSelection:
            write_booking_info(req);
            WRITE(req->conn_fd, WRITE_SEAT_MSG, 50);
            return;

        case kPayment:
            WRITE(req->conn_fd, WRITE_SEAT_OR_EXIT_MSG, 56);
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

            if (shift_is_full(req)) {
                WRITE(req->conn_fd, FULL_MSG, 32);
            } else {
                req->status = kSeatSelection;
            }
            return 0;

        case kSeatSelection:
            if (strncmp(req->cmd, "pay", 3) == 0) {
                if (req->reserved_seat_num == 0) {
                    WRITE(req->conn_fd, NO_SEAT_MSG, 21);
                    return 0;
                }
                for (int i = 1; i <= SEAT_NUM; ++i) {
                    if (seat_status(req, i) == kReservedByThisRequest) {
                        pay_seat(req, i);
                        cancel_seat(req, i);
                    }
                }
                WRITE(req->conn_fd, BOOK_SUCC_MSG, 39);
                write_booking_info(req);
                req->status = kPayment;
                return 0;
            }

            int seat = seat_str_to_int(req->cmd, req->cmd_len);
            if (seat == -1) {
                return -1;
            }
            DEBUG("seat=%d, seat_status=%d", seat, seat_status(req, seat));
            switch (seat_status(req, seat)) {
                case kAvailable:
                    reserve_seat(req, seat);
                    break;
                case kPaid:
                    WRITE(req->conn_fd, SEAT_BOOKED_MSG, 25);
                    break;
                case kReservedByOtherProcess:
                case kReservedByOtherRequest:
                    WRITE(req->conn_fd, LOCK_MSG, 13);
                    break;
                case kReservedByThisRequest:
                    WRITE(req->conn_fd, CANCEL_MSG, 26);
                    cancel_seat(req, seat);
                    break;
            }
            return 0;

        case kPayment:
            if (strncmp(req->cmd, "seat", 4) == 0) {
                req->status = kSeatSelection;
                return 0;
            }
            return -1;
    }
}
