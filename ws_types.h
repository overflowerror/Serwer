#ifndef WS_TYPES_H
#define WS_TYPES_H

#include <stdio.h>

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

#define ANY NULL

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

#endif