#ifndef INPUT_H_
#define INPUT_H_

#include "request.h"

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

int handle_input(Request* req);

#endif  // INPUT_H_
