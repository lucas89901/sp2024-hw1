#include "shift.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

#include "common.h"
#include "request.h"

// Returns the byte in shift record file that corresponds to `seat`. `seat` is an integer in [1, 40].
static off_t seat_to_byte(const int seat) { return 2 * (seat - 1); }

static bool seat_is_reserved(const int shift_fd, const int seat) {
    off_t pos = seat_to_byte(seat);
    struct flock flock;
    flock.l_type = F_WRLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = pos;
    flock.l_len = 1;
    if (fcntl(shift_fd, F_GETLK, &flock) < 0) {
        ERR_EXIT("fcntl");
    }
    return flock.l_type == F_WRLCK;
}

SeatStatus seat_status(const int shift_fd, const int seat) {
    if (seat_is_reserved(shift_fd, seat)) {
        return CHOSEN;
    }
    off_t pos = seat_to_byte(seat);
    if (lseek(shift_fd, pos, SEEK_SET) < 0) {
        ERR_EXIT("lseek error");
        return -1;
    }
    char read_buf[1];
    if (read(shift_fd, read_buf, 1) < 0) {
        ERR_EXIT("read error");
    }
    switch (read_buf[0]) {
        case '0':
            return AVAILABLE;
        case '1':
            return PAID;
    }
    return UNKNOWN;
}

int reserve_seat(const int shift_fd, const int seat) {
    off_t pos = seat_to_byte(seat);
    struct flock flock;
    flock.l_type = F_WRLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = pos;
    flock.l_len = 1;
    if (fcntl(shift_fd, F_SETLK, &flock) < 0) {
        return -1;
    }
    return 0;
}

int unlock_seat(const int shift_fd, const int seat) {
    off_t pos = seat_to_byte(seat);
    struct flock flock;
    flock.l_type = F_UNLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = pos;
    flock.l_len = 1;
    if (fcntl(shift_fd, F_SETLK, &flock) < 0) {
        return -1;
    }
    return 0;
}

int pay_seat(const int shift_fd, const int seat) {
    off_t pos = seat_to_byte(seat);
    if (lseek(shift_fd, pos, SEEK_SET) < 0) {
        ERR_EXIT("lseek error");
    }
    if (write(shift_fd, "1", 1) < 0) {
        ERR_EXIT("write");
    }
    return 0;
}

int join_seats(char* buf, Request* const req, SeatStatus status) {
    bool first = true;
    int pos = 0;
    for (int i = 1; i <= SEAT_NUM; ++i) {
        if (req->seats[i] == status) {
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

bool shift_is_full(const int shift_fd) {
    for (int i = 1; i <= SEAT_NUM; ++i) {
        SeatStatus status = seat_status(shift_fd, i);
        if (status == AVAILABLE) {
            return false;
        }
    }
    return true;
}

int lock_shift(const int shift_fd) {
    struct flock flock;
    flock.l_type = F_RDLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = 0;
    flock.l_len = 0;
    if (fcntl(shift_fd, F_SETLK, &flock) < 0) {
        return -1;
    }
    return 0;
}

int unlock_shift(const int shift_fd) {
    struct flock flock;
    flock.l_type = F_UNLCK;
    flock.l_whence = SEEK_SET;
    flock.l_start = 0;
    flock.l_len = 0;
    if (fcntl(shift_fd, F_SETLK, &flock) < 0) {
        return -1;
    }
    return 0;
}

int print_shift(const Request* const req) {
    char write_buf[SHIFT_INFO_SIZE];
    memset(write_buf, 0, SHIFT_INFO_SIZE);

    for (int i = 1; i <= SEAT_NUM; ++i) {
        char c;
        switch (seat_status(req->shift_fd, i)) {
            case AVAILABLE:
                c = '0';
                break;
            case PAID:
                c = '1';
                break;
            case CHOSEN:
                c = '2';
                break;
        }
        sprintf(write_buf + seat_to_byte(i), "%c%c", c, " \n"[i % 4 == 0]);
    }

    if (write(req->conn_fd, write_buf, SHIFT_INFO_SIZE) < 0) {
        ERR_EXIT("print shift info");
    }
    return 0;
}
