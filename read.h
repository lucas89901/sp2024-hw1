#ifndef READ_H_
#define READ_H_

#include "request.h"

int handle_read_request(Request* req, const int* shift_fds);

#endif  // READ_H_
