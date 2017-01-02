#include "serwer.h"
#include "ws_types.h"
#include "ws_utils.h"
#include "ws_modes.h"
#include "help.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>

handler_t ws_handler_find(webserver_t* server, const char* path, const char* host) {
	for (int i = 0; i < server->nrhandles; i++) {
		handle_t handle = server->handles[i];

		if (handle.host != NULL) {
			if (!ws_host_match(host, handle.host))
				continue;
		}
		if (handle.path != NULL) {
			if (!ws_path_match(path, handle.path))
				continue;
		}
		return handle.handler;
	}
	return NULL;
}

headers_t ws_headers_create(void) {
	headers_t headers;
	headers.fields = malloc(0 * sizeof(header_t));
	headers.nrfields = 0;
	return headers;
}

void ws_headers_add(headers_t* headers, const char* key, const char* value) {
	headers->fields = realloc(headers->fields, ++(headers->nrfields) * sizeof(header_t));
	headers->fields[headers->nrfields - 1].key = key;
	headers->fields[headers->nrfields - 1].value = value;
}

void ws_headers_convert(headers_t* headers, char* line) {
	const char* key = line;
	const char* value = NULL;
	bool foundSeperator = false;
	for (int i = 0; line[i] != '\0'; i++) {
		if (foundSeperator) {
			if (value == NULL) {
				if (line[i] != ' ') {
					value = line + i;
				}
			} else {
				if (line[i] == '\r' || line[i] == '\n') {
					line[i] = '\0';
					break;
				}
			}
		} else {
			if (line[i] == ':') {
				line[i] = '\0';
				foundSeperator = true;		
			}
		}
	}

	if (value != NULL)
		ws_headers_add(headers, key, value);
}

void ws_headers_free(headers_t* headers) {
	free(headers->fields);
	headers->fields = NULL;
	headers->nrfields = 0;
}

int ws_listen(webserver_t* server) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(server->host, server->port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "%s: ws_bind: getaddrinfo: %s\n", progname, gai_strerror(s));
		bail_out(EXIT_FAILURE, NULL);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		server->sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (server->sfd == -1)
			continue;

		const int on = 1;
		setsockopt(server->sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));		
		if (bind(server->sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; // Success
		close(server->sfd);
	}

	if (rp == NULL)
		return -1;

	if (listen(server->sfd, server->options.maxconnections) < 0)
		return -1;

	freeaddrinfo(result);

	return 0;
}

void ws_handle_add(webserver_t* server, handle_t handle) {
	server->handles = realloc(server->handles, ++(server->nrhandles) * sizeof(handle_t));
	server->handles[server->nrhandles - 1] = handle;
}

webserver_t ws_create(const char* name, const char* host, const char* port, FILE* logfile, srvoptions_t options) {
	webserver_t server;
	server.name = name;
	server.host = host;
	server.port = port;
	server.logfile = logfile;
	server.nrhandles = 0;
	server.handles = malloc(0);
	server.options.mode = LINEAR;
	server.options.timeout = 30;

	server.options = options;

	if (ws_listen(&server) < 0)
		bail_out(EXIT_FAILURE, "ws_listen");
	return server;
}

void ws_log(webserver_t* server, loglevel_t loglevel, const char* format, ...) {
	if (loglevel > server->options.loglevel)
		return;

	if (server->logfile == NULL)
		return;

	time_t rawtime;
	struct tm* timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char buffer[26];
	strftime(buffer, 26, "%Y.%m.%d-%H:%M:%S", timeinfo);

	va_list arg;

	fprintf(server->logfile, "%s: %s: ", buffer, server->name);

	va_start(arg, format);
	vfprintf(server->logfile, format, arg);
	va_end(arg);
	fprintf(server->logfile, "\n");
}

void ws_send(int connfd, int code, headers_t headers, int pipefd) {
	stream_t connection = (stream_t) fdopen(connfd, "a+");

	setbuf(connection, NULL);

	fprintf(connection, "%s %i %s\r\n", WS_HTTP_VERSION, code, ws_code_reason(code));

	bool serverHeader = false;
	
	for (int i = 0; i < headers.nrfields; i++) {
		header_t header = headers.fields[i];
		if (strcmp(header.key, "Server") == 0)
			serverHeader = true;
		fprintf(connection, "%s: %s\r\n", header.key, header.value);
	}
	if (!serverHeader)
		fprintf(connection, "Server: %s %s\r\n", WS_NAME, WS_VERSION);

	fprintf(connection, "\r\n");
	
	fflush(connection);

	if (pipefd == -1) {
		fclose(connection);
		return;
	}

	char buffer[BUFFER_SIZE];
	int bs = 0;
	while ((bs = read(pipefd, buffer, BUFFER_SIZE)) > 0)
		write(connfd, buffer, bs);
	
	fclose(connection);
}

void ws_simple_status(int connfd, int code) {
	ws_send(connfd, code, (headers_t) {
		.fields = NULL, 
		.nrfields = 0
	}, -1);
}


int ws_run(webserver_t* server) {
	switch(server->options.mode) {
	case LINEAR:
		return ws_run_linear(server);
	default:
		errno = EBADRQC;
		return -1;
	}
}
