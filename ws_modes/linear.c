#include "../serwer.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int ws_run_linear(webserver_t* server) {
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	int s;
	int connfd;

	char* buffer = malloc(BUFFER_SIZE * sizeof(char));
	int buffersize = 0;
	int nb = 1;

	char* header = malloc(BUFFER_SIZE * sizeof(char));
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
				buffer = realloc(buffer, nb * BUFFER_SIZE * sizeof(char));
			}

			buffer[buffersize] = '\0';

			if (buffer[buffersize - 1] == '\n') {
				ws_log(server, LOG_TESTING, "got line: %i, %s", buffersize, buffer);

				// line end
				if (buffersize == 1) {
					ws_simple_status(connfd, 400);
					break;
				}
				if (buffer[buffersize - 2] != '\r') {
					ws_simple_status(connfd, 400);
					break;
				}
				// buffer contains the line + \r\n\0

				if (buffersize == 3 - 1) { // "\r\n" + \0
					ws_log(server, LOG_TESTING, "got last header line");

					if (headerlines < 1) {
						ws_simple_status(connfd, 400);
						break;
					}

					ws_log(server, LOG_TESTING, "find host");
					host = ws_host_find(&path, headers);
					ws_log(server, LOG_DEBUG, "uri: %s%s", host, path);
		
					// find handler
					ws_log(server, LOG_TESTING, "find handler");
					handler_t* handler = ws_handler_find(server, path, host);

					if (handler == NULL) {
						ws_log(server, LOG_DEBUG, "handler not found");				
						ws_simple_status(connfd, 404); // TODO add content
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
					int code = (*handler)(*server, method, host, path, headers, &responseHeaders, request, response);
					ws_log(server, LOG_TESTING, "handler finished with %i", code);
				
					fclose(response);

					// send
					ws_log(server, LOG_TESTING, "sending response");
					size_t response_size = ws_send(connfd, code, responseHeaders, pipefd[0]);

					// cleanup
					ws_log(server, LOG_DEBUG, "cleanup");
					close(pipefd[0]);
					fclose(request);
					ws_headers_free(&responseHeaders);
					
					ws_log(server, LOG_TESTING, "done");

					if (s == 0)
						ws_log(server, LOG_MESSAGE, "%s:%s - %s %s @ %s, %i, %i bytes", phost, service, ws_method_string(method), path, host, code, response_size);
					else
						ws_log(server, LOG_MESSAGE, "-:- - %s %s @ %s, %i, %i bytes", ws_method_string(method), path, host, code, response_size);

					break;
				}				

				ws_log(server, LOG_TESTING, "enlarging header buffer");
				while (headersize + buffersize + 1 > nhb * BUFFER_SIZE) {
					nhb++;
					header = realloc(header, nhb * BUFFER_SIZE * sizeof(char));
				}

				memcpy(header + headersize, buffer, buffersize + 1);
				header[headersize + buffersize] = '\0';
				// new line is now permanently saved in header + headersize

				if (headerlines++ == 0) {
					ws_log(server, LOG_TESTING, "parsing first line");
					int e;
					if ((e = ws_request_parse(header + headersize, &path, &method)) < 0) {
						ws_log(server, LOG_DEBUG, "parsing error: %i", e);				
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
				buffer = realloc(buffer, BUFFER_SIZE * sizeof(char));
				nb = 1;
				buffersize = 0;
			}
		}

		header = realloc(header, BUFFER_SIZE * sizeof(char));
		nhb = 1;
		headersize = 0;

		headerlines = 0;

		ws_headers_free(&headers);	
		
		if (host != NULL) {
			free(host);
			host = NULL;		
		}

		buffer = realloc(buffer, BUFFER_SIZE * sizeof(char));
		nb = 1;
		buffersize = 0;
	}

	free(buffer);
	free(header);
}
