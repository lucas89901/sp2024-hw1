#ifndef COMMON_H_
#define COMMON_H_

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ERR_EXIT(a) \
    do {            \
        perror(a);  \
        exit(1);    \
    } while (0)

#define SHIFT_NUM 5
#define SHIFT_ID_START 902001
#define SHIFT_ID_END SHIFT_ID_START + (SHIFT_NUM - 1)
#define SEAT_NUM 40

#define FILE_LEN 50
#define MAX_MSG_LEN 512

#define DEBUG(...)                                   \
    fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                    \
    fprintf(stderr, "\n")

#endif  // COMMON_H_
