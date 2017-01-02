#include "serwer.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

method_t ws_method(const char* string) {
	if 	(strcmp(string, "OPTIONS") == 0)
		return OPTIONS;
	else if (strcmp(string, "GET") == 0)
		return GET;
	else if (strcmp(string, "HEAD") == 0)
		return HEAD;
	else if (strcmp(string, "POST") == 0)
		return POST;
	else if (strcmp(string, "PUT") == 0)
		return PUT;
	else if (strcmp(string, "DELETE") == 0)
		return DELETE;
	else if (strcmp(string, "TRACE") == 0)
		return TRACE;
	else if (strcmp(string, "CONNECT") == 0)
		return CONNECT;
	else
		return -1; // unknown method
}

const char* ws_method_string(method_t method) {
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
	default:
		return "unknown method";
	}
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


bool ws_host_match(const char* host, const char* match) {
	// only two possible positions for wildcard: begin and end
	int lenh = strlen(host);
	int lenm = strlen(match);

	if (lenm > lenh)
		return false;

	bool wildcard = false;
	int wnext = -1;
	int found = -1;

	int h = 0, m = 0;
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

int ws_request_parse(char* buffer, const char** path, method_t* method) {
	char lbuffer[9];
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
				*method = ws_method(lbuffer);
				if (*method < 0)
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
				if (position > 8)
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


char* ws_host_find(const char** path, headers_t headers) {
	char* host = NULL;
	for (int i = 0; i < headers.nrfields; i++) {
		header_t header = headers.fields[i];
		if (strcmp(header.key, "Host") == 0) {
			host =  malloc(strlen(header.value) + 1);
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
		host = malloc(strlen(*path) + 1);
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
		host = realloc(host, strlen(host) + 1);
	}

	return host;
}

