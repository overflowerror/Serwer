#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "ws_types.h"
#include "ws_modes.h"
#include "ws_utils.h"

#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define WS_HTTP_VERSION "HTTP/1.1"
#define WS_VERSION "0.1"
#define WS_NAME "Serwer"

#define LOG_TESTING 11
#define LOG_DEBUG 10
#define LOG_VERBOSE 5
#define LOG_WARN 2
#define LOG_MESSAGE 1
#define LOG_ERROR 0

headers_t ws_headers_create(void);
void ws_headers_add(headers_t*, const char*, const char*);
void ws_headers_convert(headers_t*, char*);
void ws_headers_free(headers_t*);

void ws_handle_add(webserver_t*, handle_t);
handler_t ws_handler_find(webserver_t*, const char*, const char*);

size_t ws_send(int, int, headers_t, int);
void ws_simple_status(int, int);

webserver_t ws_create(const char*, const char*, const char*, FILE*, srvoptions_t);
int ws_listen(webserver_t*);
int ws_run(webserver_t*);

void ws_log(webserver_t*, loglevel_t, const char*, ...);

#endif
