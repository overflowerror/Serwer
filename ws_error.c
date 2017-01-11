#include "ws_error.h"

#include <string.h>
#include <netdb.h>

error_t ws_error = {
	.type = WS,
	.no = 0
};

const char* ws_strerror(void) {
	if (ws_error.type == ERRNO || ws_error.no == EWS_SUC)
		return strerror(ws_error.no);

	if (ws_error.type == GAI)
		return gai_strerror(ws_error.no);

	// TODO

	return "undefined";
}
