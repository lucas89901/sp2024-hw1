#include "write.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "io.h"
#include "shift.h"

static int print_booking_info(Request* const req) {
    char buf[1000];
    char chosen_seats[200];
    char paid_seats[200];
    memset(buf, 0, sizeof(buf));
    join_seats(chosen_seats, req, CHOSEN);
    join_seats(paid_seats, req, PAID);

    sprintf(buf,
            "\nBooking info\n"
            "|- Shift ID: %d\n"
            "|- Chose seat(s): %s\n"
            "|- Paid: %s\n\n",
            902001 + req->shift_id, chosen_seats, paid_seats);
    if (write(req->conn_fd, buf, sizeof(buf)) < 0) {
        ERR_EXIT("write error");
    }
    return 0;
}

int handle_write_request(Request* const req, const int* const shift_fds) {
    int ret;
    while (1) {
        // Select a shift.
        if (write(req->conn_fd, WRITE_SHIFT_MSG, 59) < 0) {
            ERR_EXIT("write");
        }
        ret = handle_input(req);
        if (ret <= 0) {
            return ret;
        }
        if (!is_valid_shift(req->buf, req->buf_len)) {
            if (write(req->conn_fd, INVALID_OP_MSG, 24) < 0) {
                ERR_EXIT("write error");
            }
            return -1;
        }
        req->shift_id = req->buf[5] - '1';
        req->shift_fd = shift_fds[req->shift_id];

        if (shift_is_full(req->shift_fd)) {
            if (write(req->conn_fd, FULL_MSG, 32) < 0) {
                ERR_EXIT("write");
            }
            continue;
        }

        while (1) {  // Select a seat.
            while (1) {
                print_booking_info(req);
                if (write(req->conn_fd, WRITE_SEAT_MSG, 50) < 0) {
                    ERR_EXIT("write");
                }
                ret = handle_input(req);
                if (ret <= 0) {
                    return ret;
                }
                if (strncmp(req->buf, "pay", 3) == 0) {
                    if (req->num_chosen_seats == 0) {
                        if (write(req->conn_fd, NO_SEAT_MSG, 21) < 0) {
                            ERR_EXIT("write");
                        }
                        continue;
                    }
                    break;
                }

                int seat = seat_to_int(req->buf, req->buf_len);
                if (seat == -1) {
                    if (write(req->conn_fd, INVALID_OP_MSG, 24) < 0) {
                        ERR_EXIT("write error");
                    }
                    return -1;
                }
                switch (seat_status(req->shift_fd, seat)) {
                    case AVAILABLE:
                        // Since a process could never test for its own lock via `fcntl()`, we need to test
                        // `req->seats[seat]`.
                        switch (req->seats[seat]) {
                            case UNKNOWN:
                            case AVAILABLE:
                                req->seats[seat] = CHOSEN;
                                reserve_seat(req->shift_fd, seat);
                                ++req->num_chosen_seats;
                                break;
                            // Previously locked by this process.
                            case CHOSEN:
                                // Cancel.
                                if (write(req->conn_fd, CANCEL_MSG, 26) < 0) {
                                    ERR_EXIT("write");
                                }
                                req->seats[seat] = AVAILABLE;
                                unlock_seat(req->shift_fd, seat);
                                --req->num_chosen_seats;
                                break;
                        }
                        break;
                    case PAID:
                        if (write(req->conn_fd, SEAT_BOOKED_MSG, 25) < 0) {
                            ERR_EXIT("write");
                        }
                        break;
                    // Locked by another process.
                    case CHOSEN:
                        if (write(req->conn_fd, LOCK_MSG, 13) < 0) {
                            ERR_EXIT("write");
                        }
                        break;
                }
            }

            // Payment.
            for (int i = 1; i <= SEAT_NUM; ++i) {
                if (req->seats[i] == CHOSEN) {
                    pay_seat(req->shift_fd, i);
                    req->seats[i] = PAID;
                    unlock_seat(req->shift_fd, i);
                }
            }
            req->num_chosen_seats = 0;

            if (write(req->conn_fd, BOOK_SUCC_MSG, 39) < 0) {
                ERR_EXIT("write");
            }
            print_booking_info(req);
            if (write(req->conn_fd, WRITE_SEAT_OR_EXIT_MSG, 56) < 0) {
                ERR_EXIT("write");
            }
            ret = handle_input(req);
            if (ret <= 0) {
                return ret;
            }
            if (strncmp(req->buf, "seat", 4) == 0) {
                continue;
            }
            if (write(req->conn_fd, INVALID_OP_MSG, 24) < 0) {
                ERR_EXIT("write error");
            }
            return -1;
        }
    }
}
