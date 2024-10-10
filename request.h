#ifndef REQUEST_H_
#define REQUEST_H_

#include <sys/time.h>
#include <sys/types.h>

#include "common.h"

typedef enum RequestStatus {
    INVALID,  // Invalid state
    SHIFT,    // Shift selection
    SEAT,     // Seat selection
    BOOKED,   // Payment
} RequestStatus;

typedef enum SeatStatus {
    UNKNOWN,    // Seat is unknown
    AVAILABLE,  // Seat is available
    CHOSEN,     // Seat is currently being reserved
    PAID,       // Seat is already paid for
} SeatStatus;

typedef struct Request {
    char host[512];  // client's host
    int conn_fd;     // fd to talk with client
    int client_id;   // client's id

    char buf[MAX_MSG_LEN];  // data sent by/to client
    size_t buf_len;         // bytes used by buf

    RequestStatus status;  // request status

    int shift_id;  // [0, 4]
    int shift_fd;
    int num_chosen_seats;            // num of chosen seats
    SeatStatus seats[SEAT_NUM + 1];  // seat status

    struct timeval remaining_time;  // connection remaining time
} Request;

// initailize a request instance
void init_request(Request* req);

#endif  // REQUEST_H_
