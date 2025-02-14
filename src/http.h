#ifndef HTTP_H
#define HTTP_H

#include <signal.h>

#define BUFFER_SIZE 1024
// Specify the backlog queue size for the socket.
//
// MAX_CONNECTIONS is not a limit on concurrent connections processed.
// It does not restrict the number of connections that a multi-threaded server can handle simultaneously.
//
// MAX_CONNECTIONS only limits the number of connection requests pending in the backlog queue before being accepted by
// accept(). Once a connection is accepted by accept(), it is handled by a separate thread and is no longer subject to
// the MAX_CONNECTIONS limit.
//
// The optimal value for MAX_CONNECTIONS depends on the estimated server load.
#define MAX_CONNECTIONS 16
#define PORT 10030

extern volatile sig_atomic_t running;

void conn_wrapper(void *arg);

#endif  // HTTP_H
