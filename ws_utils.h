#ifndef WS_UTILS_H
#define WS_UTILS_H

#include "ws_types.h"

method_t ws_meth(const char*);
const char* ws_strm(method_t);
const char* ws_code_reason(int);

bool ws_host_match(const char*, const char*);
bool ws_path_match(const char*, const char*);

int ws_request_parse(char*, const char**, method_t*);

char* ws_host_find(const char**, headers_t);

#endif
