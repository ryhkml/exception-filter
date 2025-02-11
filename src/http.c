#include "http.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static void send_res(int client_socket, int status_code) {
    char status_line[BUFFER_SIZE];
    char res_header[BUFFER_SIZE];
    char res[BUFFER_SIZE * 2];
    // Buffer for formatted date strings
    char date_str_header[BUFFER_SIZE];
    char date_str[128];

    const char *status_message = NULL;

    switch (status_code) {
        case 204:
            status_message = "No Content";
            break;
            // 4xx
        case 400:
            status_message = "Bad Request";
            break;
        case 401:
            status_message = "Unauthorized";
            break;
        case 403:
            status_message = "Forbidden";
            break;
        case 404:
            status_message = "Not Found";
            break;
        case 405:
            status_message = "Method Not Allowed";
            break;
        case 413:
            status_message = "Content Too Large";
            break;
        case 429:
            status_message = "Too Many Requests";
            break;
            // 5xx
        case 500:
            status_message = "Internal Server Error";
            break;
        case 502:
            status_message = "Bad Gateway";
            break;
        case 503:
            status_message = "Service Unavailable";
            break;
        default:
            status_message = "Internal Server Error";
            status_code = 500;
            break;
    }

    int status_line_len, res_header_len, res_date_header_len, res_len;

    status_line_len = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %d %s\r\n", status_code, status_message);
    if (status_line_len < 0 || status_line_len >= (int)sizeof(status_line)) {
        fprintf(stderr, "Status line too long or encoding error\n");
        return;
    }

    // Format the time into a string according to RFC 7231 format
    time_t today = time(NULL);
    struct tm *tm_gmt = gmtime(&today);
    if (tm_gmt == NULL) {
        fprintf(stderr, "There was en error gmtime\n");
        return;
    }
    if (strftime(date_str, sizeof(date_str), "%a, %d %b %Y %H:%M:%S GMT", tm_gmt) == 0) {
        fprintf(stderr, "Failed to format date\n");
        return;
    }
    res_date_header_len = snprintf(date_str_header, sizeof(date_str_header), "Date: %s\r\n", date_str);
    if (res_date_header_len < 0 || res_date_header_len >= (int)sizeof(date_str_header)) {
        fprintf(stderr, "Date response header error\n");
        return;
    }

    res_header_len = snprintf(res_header, sizeof(res_header),
                              "Cache-Control: no-cache, no-store\r\n"
                              "Connection: close\r\n"
                              "Content-Length: 0\r\n"
                              "%s"
                              "\r\n",
                              date_str_header);
    if (res_header_len < 0 || res_header_len >= (int)sizeof(res_header)) {
        fprintf(stderr, "Response header too long or encoding error\n");
        return;
    }

    res_len = snprintf(res, sizeof(res), "%s%s", status_line, res_header);
    if (res_len < 0 || res_len >= (int)sizeof(res)) {
        fprintf(stderr, "Response too long or encoding error\n");
        return;
    }

    if (send(client_socket, res, res_len, 0) == -1) {
        fprintf(stderr, "Error sending response\n");
    }
}

static void handle_conn(int client_socket) {
    char buff[BUFFER_SIZE] = {0};
    char method[BUFFER_SIZE], path[BUFFER_SIZE], protocol[BUFFER_SIZE];

    int bytes_received = recv(client_socket, buff, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        if (running) {
            fprintf(stderr, "Error receiving request\n");
        }
        goto cleanup;
    }
    if (bytes_received == 0) {
        printf("Client disconnected\n");
        goto cleanup;
    }

    if (!running) {
        goto cleanup;
    }

    buff[bytes_received] = '\0';

    if (sscanf(buff, "%s %s %s", method, path, protocol) != 3) {
        send_res(client_socket, 400);
        goto cleanup;
    }

    if (strcmp(method, "GET") == 0) {
        if (strncmp(path, "/", 1) == 0 && strlen(path) > 1) {
            char *status_str = path + 1;
            long status_code = strtol(status_str, NULL, 10);
            if (status_code >= 100 && status_code < 600) {
                send_res(client_socket, (int)status_code);
            } else {
                send_res(client_socket, 400);
            }
        } else {
            send_res(client_socket, 204);
        }
    } else {
        send_res(client_socket, 405);
    }

cleanup:
    close(client_socket);
}

void *thread_handler(void *socket_desc) {
    int client_socket = *(int *)socket_desc;

    free(socket_desc);
    handle_conn(client_socket);

    return NULL;
}
