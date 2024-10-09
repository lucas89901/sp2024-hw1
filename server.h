#ifndef __SERVER_H
#define __SERVER_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
 * Feel free to edit any part of codes
 */

#define ERR_EXIT(a) \
    do {            \
        perror(a);  \
        exit(1);    \
    } while (0)

#define TRAIN_NUM 5
#define SEAT_NUM 40
#define TRAIN_ID_START 902001
#define TRAIN_ID_END TRAIN_ID_START + (TRAIN_NUM - 1)
#define FILE_LEN 50
#define MAX_MSG_LEN 512

enum State {
    INVALID,  // Invalid state
    SHIFT,    // Shift selection
    SEAT,     // Seat selection
    BOOKED    // Payment
};

enum Seat {
    UNKNOWN,  // Seat is unknown
    CHOSEN,   // Seat is currently being reserved
    PAID      // Seat is already paid for
};

typedef struct {
    char hostname[512];   // server's hostname
    unsigned short port;  // port to listen
    int listen_fd;        // fd to wait for a new connection
} Server;

typedef struct {
    int shift_id;                   // shift id 902001-902005
    int train_fd;                   // train file fds
    int num_chosen_seats;           // num of chosen seats
    enum Seat seat_stat[SEAT_NUM];  // seat status
} Record;

typedef struct {
    char host[512];                 // client's host
    int conn_fd;                    // fd to talk with client
    int client_id;                  // client's id
    char buf[MAX_MSG_LEN];          // data sent by/to client
    size_t buf_len;                 // bytes used by buf
    enum State status;              // request status
    Record booking_info;            // booking status
    struct timeval remaining_time;  // connection remaining time
} Request;

typedef struct {
    int file_fd;  // fd of file
} TrainInfo;

#endif
