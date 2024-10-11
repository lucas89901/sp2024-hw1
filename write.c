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
            write_message(req, kWriteShiftPrompt);
            return;
        case kSeatSelection:
            write_booking_info(req);
            write_message(req, kWriteSeatPrompt);
            return;
        case kPostPayment:
            write_message(req, kWritePostPaymentPrompt);
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
                write_message(req, kShiftFull);
            } else {
                req->status = kSeatSelection;
            }
            return 0;

        case kSeatSelection:
            if (strncmp(req->cmd, "pay", 3) == 0) {
                if (req->reserved_seat_num == 0) {
                    write_message(req, kPaymentNoSeat);
                    return 0;
                }
                for (int i = 1; i <= SEAT_NUM; ++i) {
                    if (seat_status(req, i) == kReservedByThisRequest) {
                        pay_seat(req, i);
                        cancel_seat(req, i);
                    }
                }
                write_message(req, kPaymentSuccess);
                write_booking_info(req);
                req->status = kPostPayment;
                return 0;
            }

            int seat = seat_str_to_int(req->cmd, req->cmd_len);
            if (seat == -1) {
                return -1;
            }
            LOG("seat=%d, seat_status=%d", seat, seat_status(req, seat));
            switch (seat_status(req, seat)) {
                case kAvailable:
                    reserve_seat(req, seat);
                    break;
                case kPaid:
                    write_message(req, kSeatBooked);
                    break;
                case kReservedByOtherProcess:
                case kReservedByOtherRequest:
                    write_message(req, kSeatLocked);
                    break;
                case kReservedByThisRequest:
                    write_message(req, kSeatCancel);
                    cancel_seat(req, seat);
                    break;
            }
            return 0;

        case kPostPayment:
            if (strncmp(req->cmd, "seat", 4) == 0) {
                req->status = kSeatSelection;
                return 0;
            }
            return -1;
    }
}
