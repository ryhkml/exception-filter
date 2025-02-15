#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"
#include "tp.h"

volatile sig_atomic_t running = true;

thread_pool_t *thread_pool;

static unsigned int set_uint(const char *opt, unsigned int def_v) {
    char *endptr;
    errno = 0;
    unsigned long new_v = strtoul(opt, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        return def_v;
    }
    return (unsigned int)new_v;
}

static void handle_sigact() { running = false; }

int main(int argc, const char *argv[]) {
    uint16_t port = PORT;
    unsigned int max_thread = thread_count();
    unsigned int max_queue = MAX_QUEUE;
    unsigned int max_conn = MAX_CONNECTIONS;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = (uint16_t)atoi(argv[i + 1]);
            if (port == 0) port = PORT;
        } else if (strcmp(argv[i], "--max-thread") == 0 && i + 1 < argc) {
            max_thread = set_uint(argv[i + 1], max_thread);
        } else if (strcmp(argv[i], "--max-queue") == 0 && i + 1 < argc) {
            max_queue = set_uint(argv[i + 1], max_queue);
        } else if (strcmp(argv[i], "--max-conn") == 0 && i + 1 < argc) {
            max_conn = set_uint(argv[i + 1], max_queue);
        }
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigact;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        return EXIT_SUCCESS;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        fprintf(stderr, "Socket creation failed\n");
        return EXIT_FAILURE;
    }

    // Set SO_REUSEADDR option to allow ports to be reused immediately after the server is stopped
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_fd);
        return EXIT_SUCCESS;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(server_fd);
        return EXIT_FAILURE;
    }

    if (listen(server_fd, max_conn) < 0) {
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Exception filter server listening on http://127.0.0.1:%d\n", port);

    thread_pool = thread_pool_create(max_thread, max_queue);
    if (thread_pool == NULL) {
        fprintf(stderr, "Failed to create thread pool\n");
        close(server_fd);
        return EXIT_FAILURE;
    }

    socklen_t addr_len = sizeof(addr);
    while (running) {
        int client_socket = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
        if (client_socket < 0) {
            if (errno == EINTR) {
                break;
            }
            continue;
        }

        if (!running) {
            close(client_socket);
            break;
        }

        int *new_sock = malloc(sizeof(int));
        if (new_sock == NULL) {
            printf("Failed to allocate memory for client socket\n");
            close(client_socket);
            continue;
        }
        *new_sock = client_socket;

        if (thread_pool_add_task(thread_pool, conn_wrapper, (void *)new_sock) != 0) {
            fprintf(stderr, "Failed to add task to thread pool\n");
            free(new_sock);
            close(client_socket);
        }
    }

    thread_pool_destroy(thread_pool);
    close(server_fd);

    return EXIT_SUCCESS;
}
