#ifndef INPUT_H_
#define INPUT_H_

typedef struct Request Request;
typedef struct Shift Shift;

extern const char* const INVALID_OP_MSG;
extern const char* const EXIT_MSG;

extern const char* const FULL_MSG;
extern const char* const LOCK_MSG;
extern const char* const SEAT_BOOKED_MSG;
extern const char* const CANCEL_MSG;
extern const char* const NO_SEAT_MSG;
extern const char* const BOOK_SUCC_MSG;

extern const char* const READ_SHIFT_MSG;
extern const char* const WRITE_SHIFT_MSG;
extern const char* const WRITE_SEAT_MSG;
extern const char* const WRITE_SEAT_OR_EXIT_MSG;

// If `s` represents a valid seat number, converts it to an integer and returns it. Otherwise returns -1.
int seat_str_to_int(const char* s, int len);

// If `s` represents a valid shift number, converts it to its corresponding id and returns it. Otherwise returns -1.
int shift_str_to_id(const char* s, int len);

// Returns indexs of all seats whose reserver or payer is `req`, joined into a string with ','. `req` is identified
// by its `client_id`.
void join_seats(char* buf, const int* holders, const Request* req);

// Prints the status of a shift into `buf`.
void print_shift(char* buf, const Request* req);

// Tries to read a command from the connection, and moves it into `req->buf`.
// Returns the number of bytes moved if successful, 0 if reached EOF (meaning client is down), and -1 if an error has
// occurred.
int read_command(Request* req);

#define READ_COMMAND(req)                  \
    do {                                   \
        int ret = read_command(req);       \
        DEBUG("read_command ret=%d", ret); \
        if (ret <= 0) {                    \
            return ret;                    \
        }                                  \
    } while (0)

#endif  // INPUT_H_
