#ifndef SHIFT_H_
#define SHIFT_H_

#include <stdbool.h>

#include "common.h"

typedef struct Request Request;

typedef struct Shift {
    int id;  // [0, SHIFT_NUM]

    int fd;  // File descriptor to the file holding the booking info of this shift.

    // Since a process could never test for its own lock via `fcntl()`, we need to maintain a table of seats that are
    // reserved by requests of this process.
    int reserver[SEAT_NUM + 1];

    int payer[SEAT_NUM + 1];
} Shift;

typedef enum SeatStatus {
    kAvailable = 0,
    kPaid = 1,
    kReservedByOtherProcess = 2,  // Another process is holding a write lock to the seat.
    kReservedByThisRequest = 3,
    kReservedByOtherRequest = 4,
} SeatStatus;

// Returns the byte in shift record file that corresponds to `seat`. `seat` is an integer in [1, 40].
off_t seat_to_byte(int seat);

SeatStatus seat_status(const Request* shift, int seat);
bool shift_is_full(const Request* req);

void reserve_seat(Request* req, int seat);
void cancel_seat(Request* req, int seat);
void pay_seat(Request* req, int seat);

// int lock_shift(int shift_fd);
// int unlock_shift(int shift_fd);

#endif  // SHIFT_H_
