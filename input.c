#include "input.h"

#include <stdio.h>
#include <string.h>

#include "request.h"

static const char* const EXIT_MSG = ">>> Client exit.\n";
static const unsigned char IAC_IP[3] = "\xff\xf4";

// Reads from the conenction, and moves the data read into `req->buf`.
// Returns 1 if data is successfully read, 0 if reached EOF (meaning client is down), and -1 if an error has occurred.
int handle_input(Request* req) {
    char buf[MAX_MSG_LEN + 1];
    memset(buf, 0, sizeof(buf));

    // Read in request from client.
    int ret = read(req->conn_fd, buf, sizeof(buf));
    if (ret < 0) {
        return -1;
    }
    if (ret == 0) {
        return 0;
    }

    // Look for line endings.
    char* p1 = strstr(buf, "\r\n");  // \r\n
    if (p1 == NULL) {
        p1 = strstr(buf, "\n");  // \n
        if (p1 == NULL) {
            if (!strncmp(buf, IAC_IP, 2)) {
                // Client presses ctrl+C, regard as disconnection
                fprintf(stderr, "Client pressed Ctrl+C....\n");
                return 0;
            }
        }
    }

    size_t len = p1 - buf + 1;
    memmove(req->buf, buf, len);
    req->buf[len - 1] = '\0';
    req->buf_len = len - 1;
    DEBUG("req->buf=%s,req->buf_len=%ld", req->buf, req->buf_len);

    if (strncmp(req->buf, "exit", 4) == 0) {
        if (write(req->conn_fd, EXIT_MSG, 18) < 0) {
            ERR_EXIT("write error");
        }
        return 0;
    }

    return 1;
}
