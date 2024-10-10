#ifndef READ_H_
#define READ_H_

typedef struct Request Request;
typedef struct Shift Shift;

int handle_read_request(Request* req, Shift* shifts);

#endif  // READ_H_
