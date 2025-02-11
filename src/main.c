#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"

volatile sig_atomic_t running = true;

void handle_sigact(int sig) {
    (void)sig;
    running = false;
}

int main(int argc, const char *argv[]) {
    uint16_t port = 10030;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = (uint16_t)atoi(argv[i + 1]);
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

    if (listen(server_fd, MAX_CONNECTIONS) < 0) {
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Exception filter server listening on http://127.0.0.1:%d\n", port);

    pthread_t thread_id;
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
            printf("Failed to allocate memory for socket descriptor\n");
            close(client_socket);
            continue;
        }
        *new_sock = client_socket;

        if (pthread_create(&thread_id, NULL, thread_handler, (void *)new_sock) < 0) {
            fprintf(stderr, "Failed to create thread\n");
            free(new_sock);
            close(client_socket);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(server_fd);

    return EXIT_SUCCESS;
}
