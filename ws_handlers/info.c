#include "../serwer.h"
#include "../ws_handlers.h"

#include <time.h>

int info_handler(webserver_t server, method_t method, const char* host, const char* path, headers_t requestHeaders, 
			headers_t* responseHeaders, stream_t request, stream_t response) {

	fprintf(response, "<!DOCTYPE html>\n");
	fprintf(response, "<html></head><title>Test-Page</title><style>* { font-family: Arial; }</style></head><body style=\"margin: 0px; padding: 0px; background-color: #fee\">");
	fprintf(response, "<div style=\"background-color: #eee; margin: 0 auto; height: 100%%; width: 60%%; border-left: 3px solid black; border-right: 1px solid grey; padding: 10px;\">");
	fprintf(response, "<h1 style=\"text-align: center\">Info Page for %s</h1>", server.name);

	char buffer[26];
	strftime(buffer, 26, "%Y.%m.%d %H:%M:%S", server.started);

	fprintf(response, "<h2>Server Infos</h2>");
	fprintf(response, "This server is running %s (%s Version %s) on %s:%s since %s.</br></br>", server.name, WS_NAME, WS_VERSION, server.host == NULL ? "8.8.8.8" : server.host, server.port, buffer);
	fprintf(response, "There are %i handles registered:<ul>", server.nrhandles);
	for (int i = 0; i < server.nrhandles; i++) {
		char* hhost = server.handles[i].host;
		char* hpath = server.handles[i].path;
		fprintf(response, "<li>%s %s</li>", hhost == NULL ? "(null)" : hhost, hpath = NULL ? "(null)" : hpath);
	}
	fprintf(response, "</ul>");

	fprintf(response, "<h2>Request Infos</h2>");
	fprintf(response, "Method: %s URI: %s%s</br></br>", ws_method_string(method), host, path);
	fprintf(response, "Request headers:<table style=\"border: none\">");

	for (int i = 0; i < requestHeaders.nrfields; i++) {
		header_t header = requestHeaders.fields[i];
		fprintf(response, "<tr><td style=\"border: none; font-family: \\\"Times New Roman\\\", Times, serif;\">%s:</td><td style=\"font-family: \\\"Times New Roman\\\", Times, serif;\">%s</td>", header.key, header.value);
	}
	fprintf(response, "</table>");
	fprintf(response, "<br />We are responding with 200.");
	fprintf(response, "</div></body>");

	return 200;
}
