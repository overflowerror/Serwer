#include "serwer.h"
#include "ws_error.h"
#include "ws_handlers.h"
#include "help.h"
#include <stdio.h>
#include <stdlib.h>

int hello_world(webserver_t server, method_t method, const char* host, const char* path, headers_t requestHeaders, 
			headers_t* responseHeaders, stream_t request, stream_t response) {
	// we are just using response
	(void) server;
	(void) method;
	(void) host;
	(void) path;
	(void) requestHeaders;
	(void) responseHeaders;
	(void) request;

	fprintf(response, "Hello World!\n");

	return 200;
}

int main(int argc, char** argv) {
	// we are not using these
	(void) argc;
	(void) argv;

	help_init(NULL, "test");

	srvoptions_t options = {
		.mode = LINEAR,
		.timeout = 30,
		.maxconnections = 5,		
		.loglevel = LOG_WARN
	};

	webserver_t server = ws_create("test_server", ANY , "8080", stderr, options);

	ws_handle_add(&server, (handle_t) {
		.host = ANY,
		.path = "/world",
		.handler = &hello_world
	});
	ws_handle_add(&server, (handle_t) {
		.host = ANY,
		.path = "/info",
		.handler = &info_handler
	});

	if (ws_run(&server) < 0) {
		bail_out(EXIT_FAILURE, "ws_run: %s", ws_strerror());
	}
}
