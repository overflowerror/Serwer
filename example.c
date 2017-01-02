#include "webserver.h"
#include "ws_utils.h"
#include "help.h"
#include <stdio.h>
#include <stdlib.h>

int hello_world(method_t method, const char* host, const char* path, headers_t requestHeaders, 
			headers_t* responseHeaders, stream_t request, stream_t response) {

	fprintf(response, "Hello World!\n");

	return 200;
}

int test(method_t method, const char* host, const char* path, headers_t requestHeaders, 
			headers_t* responseHeaders, stream_t request, stream_t response) {

	fprintf(response, "Method: %s, URI: %s%s\n", ws_strm(method), host, path);
	fprintf(response, "Request headers:\n");
	for (int i = 0; i < requestHeaders.nrfields; i++) {
		header_t header = requestHeaders.fields[i];
		fprintf(response, "  %s - %s\n", header.key, header.value);
	}
	fprintf(response, "\n");

	return 200;
}

int main(int argc, char** argv) {
	help_init(NULL, "test");
	
	handle_t hello_handle = {
		.host = ANY,
		.path = "/world",
		.handler = &hello_world
	};
	handle_t test_handle = {
		.host = ANY,
		.path = "/test",
		.handler = &test
	};
	srvoptions_t options = {
		.mode = LINEAR,
		.timeout = 30,
		.maxconnections = 5,		
		.loglevel = LOG_DEBUG
	};

	webserver_t server = ws_create("test_server", NULL , "8080", stderr, options);

	ws_handle_add(&server, hello_handle);
	ws_handle_add(&server, test_handle);

	ws_run(&server);
}
