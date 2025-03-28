#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
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

unsigned int set_uint(const char *opt, unsigned int default_value) {
    char *endptr;
    errno = 0;
    unsigned long new_v = strtoul(opt, &endptr, 10);
    if (errno != 0 || *endptr != '\0') {
        return default_value;
    }
    return (unsigned int)new_v;
}

void handle_sigact() { running = false; }

void print_help() {
    printf("\n");
    printf("Throwing HTTP exception in Nginx\n");
    printf("\n");
    printf("Usage   : exception-filter <options?>\n");
    printf("Options :\n");
    printf("  --max-conn <uint>    Specify backlog queue size for the socket\n");
    printf("  --max-queue <uint>   Specify task in queue in thread pool\n");
    printf("  --max-thread <uint>  Specify worker threads that can run simultaneously in thread pool\n");
    printf("  --port <uint16>      Specify exception-filter port\n");
    printf("\n");
    printf("  -h, --help           Display help message\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    uint16_t port = PORT;
    unsigned int max_thread = thread_count();
    unsigned int max_queue = MAX_QUEUE;
    unsigned int max_conn = MAX_CONNECTIONS;

    struct option some_options[] = {
        {"max-conn",   required_argument, NULL, 0  },
        {"max-queue",  required_argument, NULL, 0  },
        {"max-thread", required_argument, NULL, 0  },
        {"port",       required_argument, NULL, 0  },
        {"help",       no_argument,       NULL, 'h'},
        {0,            0,                 0,    0  }
    };

    int opt_long;
    int opt_index = 0;
    while ((opt_long = getopt_long(argc, argv, "h", some_options, &opt_index)) != -1) {
        switch (opt_long) {
            case 'h':
                print_help();
                return EXIT_SUCCESS;
            case 0:
                if (opt_index == 0) max_conn = set_uint(optarg, max_conn);
                if (opt_index == 1) max_queue = set_uint(optarg, max_queue);
                if (opt_index == 2) max_thread = set_uint(optarg, max_thread);
                if (opt_index == 3) port = (uint16_t)atoi(optarg);
                break;
            default:
                printf("Unknown option. Use -h or --help for help\n");
                return EXIT_FAILURE;
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
