#include "webserver.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include "help.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <stdarg.h>

bool ws_host_match(const char* host, const char* match) {
	// only two possible positions for wildcard: begin and end
	int lenh = strlen(host);
	int lenm = strlen(match);

	if (lenm > lenh)
		return false;

	bool wildcard = false;
	int wnext = -1;
	int found = -1;

	int h = 0, int = 0;
	for (; m < lenm && h < lenh; m++, h++) {
		if (match[m] == '*') {
			wildcard = true;
			wnext = m + 1;
			found = -1;
			// current host char matches automatically
			continue;
		}

		if (wildcard) {
			if (found == -1) {
				if (match[m] != host[h])
					m--;
				else
					found = h;
			} else {
				if (match[m] != host[h]) {
					// found was negated; reset cursor positions 
					m = wnext;
					h = found + 1;
					found = -1;
				}
			}
		} else {
			if (match[m] != host[h])
				return false;
		}
	}
	// if not whole match string matches, return false
	if (m != lenm)
		return false;
	return true;
}

bool ws_path_match(const char* path, const char* match) {
	// TODO wildcards
	int lenp = strlen(path);
	int lenm = strlen(match);

	if (lenm > lenp)
		return false;

	bool wildcard = false;
	int wnext = -1;
	int found = -1;

	int p = 0, m = 0;
	for (; m < lenm && p < lenp; m++, p++) {
		if (match[m] == '*') {
			wildcard = true;
			wnext = m + 1;
			found = -1;
			// current host char matches automatically
			continue;
		}

		if (wildcard) {
			if (found < 0) {
				if (match[m] != path[p])
					m--;
				else
					found = p;
			} else {
				if (match[m] != path[p]) {
					// found was negated; reset cursor positions 
					m = wnext;
					p = found + 1;
					found = -1;
				}
			}
			
			// if directory end
			if (path[p] == '/') {
				// found is here only true if match[m] = path[p] = '/'
				if (found >= 0) {
					wildcard = false;
				} else {
					return false;
				}
			}
		} else {
			if (match[m] != path[p])
				return false;
		}
	}

	// if not whole match string used, return false
	if (m != lenm)
		return false;
	return true;
}

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

char* ws_host_find(const char** path, headers_t headers) {
	char* host = NULL;
	for (int i = 0; i < headers.nrfields; i++) {
		header_t header = headers.fields[i];
		if (strcmp(header.key, "Host") == 0) {
			host = (char*) malloc(strlen(header.value) + 1);
			memcpy(host, header.value, strlen(header.value) + 1);
			return host;
		}	
	}

	if (strlen(*path) <= strlen("http://"))
		return host;
	if ((*path)[0] == '/')
		return host;

	bool setHost = false;

	if (host == NULL) {
		setHost = true;
		host = (char*) malloc(strlen(*path) + 1);
	}
	int hposition = 0;

	int nrslash = 0;	

	size_t len = strlen(*path);
	for (size_t i = 0; i < len; i++) {
		if (nrslash < 2) {
			// http://
		} else if (nrslash < 3) {
			if (setHost) {
				host[hposition++] = (*path)[i];
				/*if ((*path)[i] == ':') {
					host[hposition - 1] = '\0';
					setHost = false;
				}*/
			}
		} else {
			// rebase path
			*path = *path + i;
			break;
		}
		
		if ((*path)[i] == '/')
			nrslash++;
	}
	if (setHost) {
		host[hposition] = '\0';
		host = (char*) realloc(host, strlen(host) + 1);
	}

	return host;
}

const char* ws_strm(method_t method) {
	switch(method) {
	case OPTIONS:
		return "OPTIONS";
	case GET:
		return "GET";
	case HEAD:
		return "HEAD";
	case POST:
		return "POST";
	case PUT:
		return "PUT";
	case DELETE:
		return "DELETE";
	case TRACE:
		return "TRACE";
	case CONNECT:
		return "CONNECT";

	}
}

int ws_request_parse(char* buffer, const char** path, method_t* method) {
	char lbuffer[8];
	int position = 0;

	int state = 0;

	size_t len = strlen(buffer);
	for(size_t i = 0; i < len; i++) {
		switch(state) {
		case 0:	// method
			if (buffer[i] != ' ') {
				lbuffer[position++] = buffer[i];
				if (position > 7)
					return -1; // method name too long
			} else {
				state++;
				lbuffer[position] = '\0';
				if 	(strcmp(lbuffer, "OPTIONS") == 0)
					*method = OPTIONS;
				else if (strcmp(lbuffer, "GET") == 0)
					*method = GET;
				else if (strcmp(lbuffer, "HEAD") == 0)
					*method = HEAD;
				else if (strcmp(lbuffer, "POST") == 0)
					*method = POST;
				else if (strcmp(lbuffer, "PUT") == 0)
					*method = PUT;
				else if (strcmp(lbuffer, "DELETE") == 0)
					*method = DELETE;
				else if (strcmp(lbuffer, "TRACE") == 0)
					*method = TRACE;
				else if (strcmp(lbuffer, "CONNECT") == 0)
					*method = CONNECT;
				else
					return -2; // unknown method
			}
			break;
		case 1: // path begin
			if (buffer[i] == ' ' || buffer[i] == '\r')
				return -3; // malformed;
			*path = buffer + i;
			state++;
			break;
		case 2: // path
			if (buffer[i] == ' ') {
				buffer[i] = '\0';
				position = 0;
				state++;
			}
			break;
		case 3: // version
			if (buffer[i] != '\r') {
				lbuffer[position++] = buffer[i];
				if (position > 7)
					return -4; // version too long
			} else {
				lbuffer[position] = '\0';
				
				// we just support HTTP 1.0 and 1.1
				if (!strcmp(lbuffer, "HTTP/1.0"))
					return 0;
				if (!strcmp(lbuffer, "HTTP/1.1"))
					return 0;

				return -5;
			}
			break;
		default:
			assert(false);
		}
	}
	return 0;
}

headers_t ws_headers_create(void) {
	headers_t headers;
	headers.fields = (header_t*) malloc(0 * sizeof(header_t));
	headers.nrfields = 0;
	return headers;
}

void ws_headers_add(headers_t* headers, const char* key, const char* value) {
	headers->fields = (header_t*) realloc(headers->fields, ++(headers->nrfields) * sizeof(header_t));
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
	server->handles = (handle_t*) realloc(server->handles, ++(server->nrhandles) * sizeof(handle_t));
	server->handles[server->nrhandles - 1] = handle;
}

webserver_t ws_create(const char* name, const char* host, const char* port, FILE* logfile, srvoptions_t options) {
	webserver_t server;
	server.name = name;
	server.host = host;
	server.port = port;
	server.logfile = logfile;
	server.nrhandles = 0;
	server.handles = (handle_t*) malloc(0);
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

const char* ws_code_reason(int code) {
	switch(code) {
	case 100:
		return "Continue";
	case 101:
		return "Switching Protocols";
	case 200:
		return "OK";

	case 201:
		return "Created";
	case 202:
		return "Accepted";
	case 203:
		return "Non-Authoritative Information";
	case 204:
		return "No Content";
	case 205:
		return "Reset Content";
	case 206:
		return "Partial Content";

	case 300:
		return "Multible Choices";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 303:
		return "See Other";
	case 304:
		return "Not Modified";
	case 305:
		return "Use Proxy";
	case 307:
		return "Temporary Redirect";

	case 400:
		return "Bad Request";	
	case 401:
		return "Unauthorized";
	case 402:
		return "Payment Required";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 406:
		return "Not Acceptable";
	case 407:
		return "Proxy Authentication Required";
	case 408:
		return "Request Time-out";
	case 409:
		return "Conflict";
	case 410:
		return "Gone";
	case 411:
		return "Length Required";
	case 412:
		return "Precondition Failed";
	case 413:
		return "Request Entify Too Large";
	case 414:
		return "Request-URI Too Large";
	case 415:
		return "Unsupported Media Type";
	case 416:
		return "Requested range not satisfiable";
	case 417:
		return "Expectation Failed";

	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	case 504:
		return "Gateway Time-out";
	case 505:
		return "HTTP Version not supported";
	default: 
		return "Unknown extension code";
	}
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
	ws_send(connfd, code, (headers_t) {.fields = NULL, .nrfields = 0}, -1);
}

int ws_run_linear(webserver_t* server) {
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	int s;
	int connfd;

	char* buffer = (char*) malloc(BUFFER_SIZE * sizeof(char));
	int buffersize = 0;
	int nb = 1;

	char* header = (char*) malloc(BUFFER_SIZE * sizeof(char));
	int headersize = 0;
	int nhb = 1;

	int headerlines = 0;

	while (true) {
		if((connfd = accept(server->sfd, (struct sockaddr*) &peer_addr, &peer_addr_len)) < 0) {
			return -1;
		}

		char phost[NI_MAXHOST], service[NI_MAXSERV];

		s = getnameinfo((struct sockaddr *) &peer_addr, 
			peer_addr_len, phost, NI_MAXHOST, service, 
			NI_MAXSERV, NI_NUMERICSERV);

		if (s == 0)
			ws_log(server, LOG_DEBUG, "Connection from %s:%s", phost, service);
		else
			ws_log(server, LOG_DEBUG, "getnameinfo: %s", gai_strerror(s));

		headers_t headers = ws_headers_create();
		char* host = NULL;
		method_t method;
		const char* path = NULL;

		while(true) {
			if (read(connfd, buffer + buffersize++, 1) < 1) {
				ws_log(server, LOG_TESTING, "read: %s", strerror(errno));
				// error or disconect
				break;
			}

			if (buffersize > nb * BUFFER_SIZE - 1) {
				nb++;
				buffer = (char*) realloc(buffer, nb * BUFFER_SIZE * sizeof(char));
			}

			buffer[buffersize] = '\0';

			if (buffer[buffersize - 1] == '\n') {
				ws_log(server, LOG_TESTING, "got line: %i, %s", buffersize, buffer);

				// line end
				if (buffersize == 1) {
					// TODO 400
					ws_simple_status(connfd, 400);
					break;
				}
				if (buffer[buffersize - 2] != '\r') {
					// TODO 400
					ws_simple_status(connfd, 400);
					break;
				}
				// buffer contains the line + \r\n\0

				if (buffersize == 3 - 1) { // "\r\n" + \0
					ws_log(server, LOG_TESTING, "got last header line");

					if (headerlines < 1) {
						// TODO 400
						ws_simple_status(connfd, 400);
						break;
					}

					ws_log(server, LOG_TESTING, "find host");
					host = ws_host_find(&path, headers);
					ws_log(server, LOG_DEBUG, "uri: %s%s", host, path);
		
					// find handler
					ws_log(server, LOG_TESTING, "find handler");
					handler_t handler = ws_handler_find(server, path, host);

					if (handler == NULL) {
						// TODO 404	
						ws_log(server, LOG_DEBUG, "handler not found");				
						ws_simple_status(connfd, 404);
						break;
					}
					ws_log(server, LOG_TESTING, "found");

					int pipefd[2];

					ws_log(server, LOG_TESTING, "create pipe");
					if (pipe(pipefd) < 0) {
						return -1;
					}	

					ws_log(server, LOG_TESTING, "create response header struct");
					headers_t responseHeaders = ws_headers_create();
					ws_log(server, LOG_TESTING, "open request stream");
					stream_t request = (stream_t) fdopen(connfd, "r");				
					
					if (request == NULL) {
						ws_log(server, LOG_DEBUG, "fdopen: %s", strerror(errno));
					}

					//fopen write end of pipe 
					ws_log(server, LOG_TESTING, "open pipe stream");
					stream_t response = (stream_t) fdopen(pipefd[1], "w"); 

					if (response == NULL) {
						ws_log(server, LOG_DEBUG, "fdopen: %s", strerror(errno));
					}
	
					// unbuffer
					setbuf(request, NULL);

					// handler
					ws_log(server, LOG_DEBUG, "execute handler");
					int code = (*handler)(method, host, path, headers, &responseHeaders, request, response);
					ws_log(server, LOG_TESTING, "handler finished with %i", code);
				
					fclose(response);

					// send
					ws_log(server, LOG_TESTING, "sending response");
					ws_send(connfd, code, responseHeaders, pipefd[0]);

					// cleanup
					ws_log(server, LOG_DEBUG, "cleanup");
					close(pipefd[0]);
					fclose(request);
					ws_headers_free(&responseHeaders);
					
					ws_log(server, LOG_TESTING, "done");

					break;
				}				

				ws_log(server, LOG_TESTING, "enlarging header buffer");
				while (headersize + buffersize + 1 > nhb * BUFFER_SIZE) {
					nhb++;
					header = (char*) realloc(header, nhb * BUFFER_SIZE * sizeof(char));
				}

				memcpy(header + headersize, buffer, buffersize + 1);
				header[headersize + buffersize] = '\0';
				// new line is now permanently saved in header + headersize

				if (headerlines++ == 0) {
					ws_log(server, LOG_TESTING, "parsing first line");
					int e;
					if ((e = ws_request_parse(header + headersize, &path, &method)) < 0) {
						ws_log(server, LOG_DEBUG, "parsing error: %i", e);
						// TODO 400						
						ws_simple_status(connfd, 400);
						break;
					}
					ws_log(server, LOG_TESTING, "path: %s", path);
				} else {
					// convert to header struct
					ws_log(server, LOG_TESTING, "convert line to headerstruct");
					ws_headers_convert(&headers, header + headersize);
					ws_log(server, LOG_DEBUG, "header: %s: %s", headers.fields[headers.nrfields - 1].key, 
											headers.fields[headers.nrfields - 1].value);
				}
				headersize += buffersize + 1;

				ws_log(server, LOG_TESTING, "reset buffer");
				buffer = (char*) realloc(buffer, BUFFER_SIZE * sizeof(char));
				nb = 1;
				buffersize = 0;
			}
		}

		header = (char*) realloc(header, BUFFER_SIZE * sizeof(char));
		nhb = 1;
		headersize = 0;

		headerlines = 0;

		ws_headers_free(&headers);	
		
		if (host != NULL)
			free(host);
		host = NULL;		

		buffer = (char*) realloc(buffer, BUFFER_SIZE * sizeof(char));
		nb = 1;
		buffersize = 0;
	}

	free(buffer);
	free(header);
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
