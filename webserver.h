#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <stdio.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define WS_HTTP_VERSION "HTTP/1.1"
#define WS_VERSION "0.1"
#define WS_NAME "SERWER"

typedef struct header {
	const char* key;
	const char* value;
} header_t;

typedef struct headers {
	header_t* fields;
	int nrfields;
} headers_t;

typedef FILE* stream_t;

typedef int loglevel_t;

#define LOG_TESTING 11
#define LOG_DEBUG 10
#define LOG_VERBOSE 5
#define LOG_WARN 2
#define LOG_ERROR 0

typedef enum method {
	OPTIONS,
	GET,
	HEAD,
	POST,
	PUT,
	DELETE,
	TRACE,
	CONNECT
} method_t;

typedef int(*handler_t)(method_t method, const char* host, const char* path, headers_t requestHeaders, 
			headers_t* responseHeaders, stream_t request, stream_t response);

typedef struct handle {
	const char* path;
	const char* host;
	handler_t handler;
} handle_t;

typedef enum mode {
	//PRE_FORKED,
	//POST_FORKED,
	//STATEFULL,
	//THREADED,
	LINEAR
} srvmode_t;

typedef struct srvoptions {
	srvmode_t mode;
	int timeout;
	int maxconnections;	
	loglevel_t loglevel;
} srvoptions_t;

typedef struct webserver {
	const char* name;
	int sfd;
	const char* host;
	const char* port;
	handle_t* handles;
	int nrhandles;
	FILE* logfile;
	srvoptions_t options;
} webserver_t;

bool ws_host_match(const char*, const char*);
bool ws_path_match(const char*, const char*);

handler_t ws_handler_find(webserver_t*, const char*, const char*);

char* ws_host_find(const char**, headers_t);

const char* ws_strm(method_t);

int ws_request_parse(char*, const char**, method_t*);

headers_t ws_headers_create(void);

void ws_headers_add(headers_t*, const char*, const char*);

void ws_headers_convert(headers_t*, char*);

void ws_headers_free(headers_t*);

int ws_listen(webserver_t*);

void ws_handle_add(webserver_t*, handle_t);

webserver_t ws_create(const char*, const char*, const char*, FILE*, srvoptions_t);

void ws_log(webserver_t*, loglevel_t, const char*, ...);

const char* ws_code_reason(int);

void ws_send(int, int, headers_t, int);

void ws_simple_status(int, int);

int ws_run_linear(webserver_t*);

int ws_run(webserver_t*);

#endif
