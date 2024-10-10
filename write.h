#ifndef WRITE_H_
#define WRITE_H_

#include "request.h"

int handle_write_request(Request* req, const int* shift_fds);

#endif  // WRITE_H_
