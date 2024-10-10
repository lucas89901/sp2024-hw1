#ifndef SHIFT_H_
#define SHIFT_H_

#include <stdbool.h>

#include "request.h"

#define SHIFT_INFO_SIZE 81

SeatStatus seat_status(int shift_fd, int seat);
int seat_to_int(const char* s, int len);
int join_seats(char* buf, Request* req, SeatStatus status);

int reserve_seat(int shift_fd, int seat);
int unlock_seat(int shift_fd, int seat);
int pay_seat(int shift_fd, int seat);

bool is_valid_shift(const char* s, int len);
bool shift_is_full(int shift_fd);
int lock_shift(int shift_fd);
int unlock_shift(int shift_fd);

int print_shift(const Request* req);

#endif  // SHIFT_H_
