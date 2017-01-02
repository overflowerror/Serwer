# Serwer
## Standalone webserver written in C99

### What is this shit?
Well, it's standalone webserver written entirely in C99 (no libs required).

### Why would someone need this?
I've no idea. But I'm going to use this for my next project.

### Why is it's name to weird?
Because W for web... Get it? ... Well next time I'll ask you for a catchy name, smartypants.

### Is this ... thing ... even finished?
No. There are many functions missing and the code needs some revision (and comments I suppose).
There is a working mode though (see [example.c](example.c) for an example).

If you want to help me with this project, feel free to fork and submit-pull requests. 
I also appreciate opinions, suggestions and complaints.

### I don't get it. Is this a webserver for a lib for webservers?
Both, I guess. Although as I'm thinking about it, it might actually be more of a library...

### Okay...? So, how can I use this thing?

See [exmaple.c](example.c).

Basically you need at least one handler.
```c
/**
 * @param method	contains the method-type (see method_t); can be stringified with const char* ws_strm(method_t)
 * @param host		the hostname (e.g. "localhost:8080" or "www.google.com")
 * @param path		the requested URI
 * @param requestHeaders	the headers of the request (see headers_t)
 * @param responseHeaders	well... guess. (Hint: You can modify these.)
 * @param request	"file" stream of the request (can be read with e.g. fscanf(2))
 * @param respinse	"file" stream for the response (can be written with w.g. fprintf(2))
 */
int not_found(method_t method, const char* host, const char* path, headers_t requestHeaders, 
		headers_t* responseHeaders, stream_t request, stream_t response) {
	// The definition of this function is defined as handler_t (don't confuse with handle_t) in /webserver.h.

	// You can write a response like this.
	fprintf(response, "<!DOCTYPE html>\n");
	fprintf(response, "<html><head><title>Not Found</title></head><body>");
	fprintf(response, "<h1>404 - File Not Found</h1>");
	fprintf(response, "<h1>The requested file (%s) was not found on this server.</h1>", path);
	fprintf(response, "</body></html>");

	return 404;	// This is the HTTP status code by the way.
}
```

Then you can setup a server.
```c
srvoptions_t options = {
	.mode = LINEAR,		// the mode to use
	.timeout = 30,		// the connection timeout (in this case useless, because of LINEAR)
	.maxconnections = 5,	// the maximum number of connections
	.loglevel = LOG_DEBUG	// the log level for the logfile
};

// webserver_t ws_create(const char* name, const char* host (can be NULL for default), const char* port, FILE* logfile, srvoptions_t options);
webserver_t server = ws_create("server_name", NULL, "8080", stderr, options);
```

Now you can add you handlers.
```c
handle_t handle = {
	.host = ANY,		// the hostname to listen on (ANY can be used)
	.path = "/*.c",		// yes, you can use wildcards (ANY can still be used)
	.handler = &not_found	// function pointer of the handler
};
ws_handle_add(&server, handle);
```

Done. Now you can start it.
```c
ws_run(&server);
```

Simple as that. : )

Any questions?

### ... Oh, you were talking to me?
-.- 

### Is there something missing?
Well, where can I start?

- the other modes (LINEAR is working, but it's not useable in practice).
- SSL/HTTPS support
- management for cookies and sessions
- index-handlers
- cgi-handlers

## Okay, anything else to do?
Yes:

- extracting the ws_*_run-functions plus their own dependencies to other files (I hate files with more than 1000 lines. D: )
- comments (Yes I know, I'm lazy on that.)
- some security features would be nice (timeouts, ...)
