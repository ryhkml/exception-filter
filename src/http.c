#include "http.h"

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static void send_res(int client_socket, int status_code) {
    const char *status_message = NULL;
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

    char *status_line_buff;
    size_t status_line_size = snprintf(NULL, 0, "HTTP/1.1 %d %s\r\n", status_code, status_message);
    status_line_buff = malloc(status_line_size + 1);
    if (!status_line_buff) {
        printf("Buy more RAM\n");
        return;
    };
    snprintf(status_line_buff, status_line_size + 1, "HTTP/1.1 %d %s\r\n", status_code, status_message);

    // Format the time into a string according to RFC 7231 format
    time_t today = time(NULL);
    struct tm *tm_gmt = gmtime(&today);
    if (!tm_gmt) {
        fprintf(stderr, "There was en error gmtime\n");
        free(status_line_buff);
        return;
    }
    size_t date_buff_init_size = 64;
    char *date_buff = malloc(date_buff_init_size);
    if (!date_buff) {
        printf("Buy more RAM\n");
        free(status_line_buff);
        return;
    }
    if (strftime(date_buff, date_buff_init_size, "%a, %d %b %Y %H:%M:%S GMT", tm_gmt) == 0) {
        printf("Failed to format date\n");
        free(date_buff);
        free(status_line_buff);
        return;
    }
    size_t date_buff_size = strlen(date_buff);
    if (date_buff_size < date_buff_init_size) {
        char *tmp = realloc(date_buff, date_buff_size + 1);
        if (!tmp) {
            printf("Failed to realloc\n");
            free(date_buff);
            free(status_line_buff);
            return;
        }
        date_buff = tmp;
    }

    char *date_header_buff;
    size_t date_header_size = snprintf(NULL, 0, "Date: %s\r\n", date_buff);
    date_header_buff = malloc(date_header_size + 1);
    if (!date_header_buff) {
        free(date_buff);
        free(status_line_buff);
        return;
    }
    snprintf(date_header_buff, date_header_size + 1, "Date: %s\r\n", date_buff);

    char *res_header_buff;
    size_t res_header_size = snprintf(NULL, 0,
                                      "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                                      "Content-Length: 0\r\n"
                                      "%s"
                                      "\r\n",
                                      date_header_buff);
    res_header_buff = malloc(res_header_size + 1);
    if (!res_header_buff) {
        printf("Buy more RAM\n");
        free(date_header_buff);
        free(date_buff);
        free(status_line_buff);
        return;
    }
    snprintf(res_header_buff, res_header_size + 1,
             "Cache-Control: no-cache, no-store, must-revalidate\r\n"
             "Content-Length: 0\r\n"
             "%s"
             "\r\n",
             date_header_buff);

    char *res_buff;
    size_t res_size = snprintf(NULL, 0, "%s%s", status_line_buff, res_header_buff);
    res_buff = malloc(res_size + 1);
    if (!res_buff) {
        printf("Buy more RAM\n");
        free(res_header_buff);
        free(date_header_buff);
        free(date_buff);
        free(status_line_buff);
        return;
    }
    snprintf(res_buff, res_size + 1, "%s%s", status_line_buff, res_header_buff);

    if (send(client_socket, res_buff, res_size, 0) == -1) {
        fprintf(stderr, "Error sending response\n");
    }

    free(res_buff);
    free(res_header_buff);
    free(date_header_buff);
    free(date_buff);
    free(status_line_buff);
}

static void handle_conn(int client_socket) {
    char path[7], method[9], protocol[17], req_headers[BUFFER_SIZE];

    int bytes_received = recv(client_socket, req_headers, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        if (running) printf("Error receiving request\n");
        goto done;
    }
    if (bytes_received == 0) {
        printf("Client disconnected\n");
        goto done;
    }

    if (!running) goto done;

    if (sscanf(req_headers, "%8s %6s %16s", method, path, protocol) != 3) {
        send_res(client_socket, 400);
        goto done;
    }

    if (strcmp(method, "GET") == 0) {
        if (strncmp(path, "/", 1) == 0 && strlen(path) > 1) {
            char *status_str = path + 1;
            uint16_t status_code = atoi(status_str);
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

done:
    close(client_socket);
}

void conn_wrapper(void *arg) {
    int client_socket = *(int *)arg;
    free(arg);
    handle_conn(client_socket);
}
