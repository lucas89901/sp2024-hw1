#ifndef WRITE_H_
#define WRITE_H_

typedef struct Request Request;
typedef struct Shift Shift;

int handle_write_request(Request* req, Shift* shifts);

#endif  // WRITE_H_
