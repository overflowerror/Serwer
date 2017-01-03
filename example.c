#include "serwer.h"
#include "ws_handlers.h"
#include "help.h"
#include <stdio.h>
#include <stdlib.h>

int hello_world(webserver_t server, method_t method, const char* host, const char* path, headers_t requestHeaders, 
			headers_t* responseHeaders, stream_t request, stream_t response) {

	fprintf(response, "Hello World!\n");

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
		.path = "/info",
		.handler = &info_handler
	};
	srvoptions_t options = {
		.mode = LINEAR,
		.timeout = 30,
		.maxconnections = 5,		
		.loglevel = LOG_WARN
	};

	webserver_t server = ws_create("test_server", ANY , "8080", stderr, options);

	ws_handle_add(&server, hello_handle);
	ws_handle_add(&server, test_handle);

	ws_run(&server);
}
