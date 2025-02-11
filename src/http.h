#ifndef HTTP_H
#define HTTP_H

#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_CONNECTIONS 16

extern volatile sig_atomic_t running;

void *thread_handler(void *socket_desc);

#endif  // HTTP_H
