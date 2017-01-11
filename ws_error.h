#ifndef WS_ERROR_H
#define WS_ERROR_H

#define EWS_SUC 0
// TODO

enum error_type {
	ERRNO,
	GAI,
	WS
};

typedef struct error {
	enum error_type type;
	int no;
} error_t;

extern error_t ws_error;

const char* ws_strerror(void);

#endif
