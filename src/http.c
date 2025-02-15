#include "http.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static void send_res(int client_socket, int status_code) {
    const char *status_message = NULL;
    char status_line[BUFFER_SIZE], res_header[BUFFER_SIZE], res[BUFFER_SIZE * 2],
        // Buffer for formatted date strings
        date_str_header[256], date_str[128];

    switch (status_code) {
        case 204:
            status_message = "No Content";
            break;
            // 3xx
        case 300:
            status_message = "Multiple Choices";
            break;
        case 301:
            status_message = "Moved Permanently";
            break;
        case 302:
            status_message = "Found";
            break;
        case 303:
            status_message = "See Other";
            break;
        case 304:
            status_message = "Not Modified";
            break;
        case 307:
            status_message = "Temporary Redirect";
            break;
        case 308:
            status_message = "Permanent Redirect";
            break;
            // 4xx
        case 400:
            status_message = "Bad Request";
            break;
        case 401:
            status_message = "Unauthorized";
            break;
        case 402:
            status_message = "Payment Required";
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
        case 406:
            status_message = "Not Acceptable";
            break;
        case 407:
            status_message = "Proxy Authentication Required";
            break;
        case 408:
            status_message = "Request Timeout";
            break;
        case 409:
            status_message = "Conflict";
            break;
        case 410:
            status_message = "Gone";
            break;
        case 411:
            status_message = "Length Required";
            break;
        case 412:
            status_message = "Precondition Failed";
            break;
        case 413:
            status_message = "Content Too Large";
            break;
        case 414:
            status_message = "URI Too Long";
            break;
        case 415:
            status_message = "Unsupported Media Type";
            break;
        case 416:
            status_message = "Range Not Satisfiable";
            break;
        case 417:
            status_message = "Expectation Failed";
            break;
        case 418:
            status_message = "I'm a teapot";
            break;
        case 421:
            status_message = "Misdirected Request";
            break;
        case 422:
            status_message = "Unprocessable Content";
            break;
        case 423:
            status_message = "Locked";
            break;
        case 424:
            status_message = "Failed Dependency";
            break;
        case 425:
            status_message = "Too Early";
            break;
        case 426:
            status_message = "Upgrade Required";
            break;
        case 428:
            status_message = "Precondition Required";
            break;
        case 429:
            status_message = "Too Many Requests";
            break;
        case 431:
            status_message = "Request Header Fields Too Large";
            break;
        case 451:
            status_message = "Unavailable For Legal Reasons";
            break;
            // 5xx
        case 500:
            status_message = "Internal Server Error";
            break;
        case 501:
            status_message = "Not Implemented";
            break;
        case 502:
            status_message = "Bad Gateway";
            break;
        case 503:
            status_message = "Service Unavailable";
            break;
        case 504:
            status_message = "Gateway Timeout";
            break;
        case 505:
            status_message = "HTTP Version Not Supported";
            break;
        case 506:
            status_message = "Variant Also Negotiates";
            break;
        case 507:
            status_message = "Insufficient Storage";
            break;
        case 508:
            status_message = "Loop Detected";
            break;
        case 510:
            status_message = "Not Extended";
            break;
        case 511:
            status_message = "Network Authentication Required";
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
                              "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                              "Connection: close\r\n"
                              "Content-Length: 0\r\n"
                              "Expires: 0\r\n"
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
    char req_headers[BUFFER_SIZE] = {0};
    char method[8], path[BUFFER_SIZE], protocol[16];

    int bytes_received = recv(client_socket, req_headers, BUFFER_SIZE - 1, 0);
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

    req_headers[bytes_received] = '\0';

    if (sscanf(req_headers, "%7s %1023s %15s", method, path, protocol) != 3) {
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

void conn_wrapper(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    handle_conn(client_socket);
}
