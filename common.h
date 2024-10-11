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

#ifdef DEBUG
#define LOG(...)                                     \
    fprintf(stderr, "[%s:%d] ", __FILE__, __LINE__); \
    fprintf(stderr, __VA_ARGS__);                    \
    fprintf(stderr, "\n")
#else
#define LOG(...)
#endif

#define READ(fd, buf, count)            \
    do {                                \
        if (read(fd, buf, count) < 0) { \
            ERR_EXIT("read");           \
        }                               \
    } while (0)

#define WRITE(fd, buf, count)            \
    do {                                 \
        if (write(fd, buf, count) < 0) { \
            ERR_EXIT("write");           \
        }                                \
    } while (0)

#define LSEEK(fd, offset, whence)            \
    do {                                     \
        if (lseek(fd, offset, whence) < 0) { \
            ERR_EXIT("lseek");               \
        }                                    \
    } while (0)

#endif  // COMMON_H_
