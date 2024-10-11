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

// Reads and handles currently available data from the request's connection. Returns 1 if the connection should remain
// open, 0 if the connection should be closed, -1 if an error has occurred.
int read_connection(Request* req, Shift* shifts);

// =========== read or write dependent ========

void write_prompt(const Request* req);

// Handles a command. Returns 0 successful, -1 if an errror has occurred.
int handle_command(Request* req, Shift* shifts);

#endif  // INPUT_H_
