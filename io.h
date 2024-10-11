#ifndef INPUT_H_
#define INPUT_H_

typedef struct Request Request;
typedef struct Shift Shift;

// If `s` represents a valid seat number, converts it to an integer and returns it. Otherwise returns -1.
int seat_str_to_int(const char* s, int len);

// If `s` represents a valid shift number, converts it to its corresponding id and returns it. Otherwise returns -1.
int shift_str_to_id(const char* s, int len);

// Returns indexs of all seats whose reserver or payer is `req`, joined into a string with ','. `req` is identified
// by its `client_id`.
void join_seats(char* buf, const int* holders, const Request* req);

typedef enum Message {
    kWelcomeBanner = 0,
    kInvalidOp,
    kExit,

    kShiftFull,
    kSeatLocked,
    kSeatBooked,
    kSeatCancel,

    kPaymentSuccess,
    kPaymentNoSeat,

    // Prompts
    kReadShiftPrompt,
    kWriteShiftPrompt,
    kWriteSeatPrompt,
    kWritePostPaymentPrompt,
} Message;

// Writes the desired message to the request's conenction.
void write_message(const Request* req, Message message);

// Reads and handles currently available data from the request's connection. Returns 1 if the connection should remain
// open, 0 if the connection should be closed, -1 if an error has occurred.
int read_connection(Request* req, Shift* shifts);

// =========== read or write dependent ========

// Writes the appropriate prompt to the connection depending on the request's state.
void write_prompt(const Request* req);

// Handles a command. Returns 0 successful, -1 if an errror has occurred.
int handle_command(Request* req, Shift* shifts);

#endif  // INPUT_H_
