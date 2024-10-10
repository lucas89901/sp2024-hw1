#include "shift.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "common.h"
#include "request.h"

off_t seat_to_byte(const int seat) { return 2 * (seat - 1); }

SeatStatus seat_status(const Request* const req, const int seat) {
    if (req->shift->reserver[seat] == req->client_id) {
        return kReservedByThisRequest;
    }
    if (req->shift->reserver[seat] != -1) {
        return kReservedByOtherRequest;
    }

    off_t pos = seat_to_byte(seat);
    // Test for write lock by other write servers.
    struct flock flock;
    flock.l_type = F_WRLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = pos;
    flock.l_len = 1;
    if (fcntl(req->shift->fd, F_GETLK, &flock) < 0) {
        ERR_EXIT("fcntl");
    }
    if (flock.l_type == F_WRLCK) {
        return kReservedByOtherProcess;
    }

    // Not reserved by anyone, so we read from file.
    char read_buf[1];
    LSEEK(req->shift->fd, pos, SEEK_SET);
    READ(req->shift->fd, read_buf, 1);
    switch (read_buf[0]) {
        case '0':
            return kAvailable;
        case '1':
            return kPaid;
    }
}

bool shift_is_full(const Request* const req) {
    for (int i = 1; i <= SEAT_NUM; ++i) {
        if (seat_status(req, i) != kPaid) {
            return false;
        }
    }
    return true;
}

void reserve_seat(Request* const req, const int seat) {
    off_t pos = seat_to_byte(seat);
    struct flock flock;
    flock.l_type = F_WRLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = pos;
    flock.l_len = 1;
    if (fcntl(req->shift->fd, F_SETLK, &flock) < 0) {
        ERR_EXIT("fcntl");
    }
    req->shift->reserver[seat] = req->client_id;
    ++req->reserved_seat_num;
}

void cancel_seat(Request* const req, const int seat) {
    off_t pos = seat_to_byte(seat);
    struct flock flock;
    flock.l_type = F_UNLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = pos;
    flock.l_len = 1;
    if (fcntl(req->shift->fd, F_SETLK, &flock) < 0) {
        ERR_EXIT("fcntl");
    }
    req->shift->reserver[seat] = -1;
    --req->reserved_seat_num;
}

void pay_seat(Request* const req, const int seat) {
    off_t pos = seat_to_byte(seat);
    LSEEK(req->shift->fd, pos, SEEK_SET);
    WRITE(req->shift->fd, "1", 1);
    req->shift->payer[seat] = req->client_id;
}

// int lock_shift(const int shift_fd) {
//     struct flock flock;
//     flock.l_type = F_RDLCK;
//     flock.l_whence = SEEK_SET;
//     flock.l_start = 0;
//     flock.l_len = 0;
//     if (fcntl(shift_fd, F_SETLK, &flock) < 0) {
//         return -1;
//     }
//     return 0;
// }

// int unlock_shift(const int shift_fd) {
//     struct flock flock;
//     flock.l_type = F_UNLCK;
//     flock.l_whence = SEEK_SET;
//     flock.l_start = 0;
//     flock.l_len = 0;
//     if (fcntl(shift_fd, F_SETLK, &flock) < 0) {
//         return -1;
//     }
//     return 0;
// }
