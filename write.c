#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "io.h"
#include "request.h"
#include "shift.h"

static int print_booking_info(Request* const req) {
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
    return 0;
}

int handle_request(Request* const req, Shift* const shifts) {
    while (1) {  // Select a shift.
        WRITE(req->conn_fd, WRITE_SHIFT_MSG, 59);

        READ_COMMAND(req);
        int shift_id = shift_str_to_id(req->buf, req->buf_len);
        if (shift_id < 0) {
            WRITE(req->conn_fd, INVALID_OP_MSG, 24);
            return -1;
        }
        req->shift = &shifts[shift_id];

        if (shift_is_full(req)) {
            WRITE(req->conn_fd, FULL_MSG, 32);
            continue;
        }

        while (1) {  // Select a seat.
            while (1) {
                print_booking_info(req);

                WRITE(req->conn_fd, WRITE_SEAT_MSG, 50);
                READ_COMMAND(req);
                if (strncmp(req->buf, "pay", 3) == 0) {
                    if (req->reserved_seat_num == 0) {
                        WRITE(req->conn_fd, NO_SEAT_MSG, 21);
                        continue;
                    }
                    break;
                }
                int seat = seat_str_to_int(req->buf, req->buf_len);
                if (seat == -1) {
                    WRITE(req->conn_fd, INVALID_OP_MSG, 24);
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
            }

            // Payment.
            for (int i = 1; i <= SEAT_NUM; ++i) {
                if (seat_status(req, i) == kReservedByThisRequest) {
                    pay_seat(req, i);
                    cancel_seat(req, i);
                }
            }
            WRITE(req->conn_fd, BOOK_SUCC_MSG, 39);
            print_booking_info(req);

            WRITE(req->conn_fd, WRITE_SEAT_OR_EXIT_MSG, 56);
            READ_COMMAND(req);
            if (strncmp(req->buf, "seat", 4) == 0) {
                continue;
            }
            WRITE(req->conn_fd, INVALID_OP_MSG, 24);
            return -1;
        }
    }
}
