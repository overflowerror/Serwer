/**
 * @file help.c
 * @author overflowerror
 * @date 2016.11.27
 * @brief code for the help functions
 * 
 * This file contains the code for this lib.
 */

#include "help.h"
#include <stdio.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>

/**
 * @see help.h
 */
const char* progname = "";

/**
 * @brief the free function
 * 
 * This function gets executes if bail_out() or usage() are called.
 */
static free_t freeFunction = NULL;

void help_init(free_t function, const char* name) {
	freeFunction = function;
	progname = name;
}

void bail_out(int exitcode, const char* format, ...) {
	(void) fprintf(stderr, "%s: ", progname);

	va_list arg;
	va_start(arg, format);
	(void) vfprintf(stderr, format, arg);
	va_end(arg);

	(void) fprintf(stderr, "\n");

	if (freeFunction != NULL)
		(*freeFunction)();
	exit(exitcode);
}

void usage(const char* arguments, const char* description, const char* error, int exitcode) {
	if (error != NULL) {
		(void) fprintf(stderr, "%s\n\n", error);
	}
	(void) fprintf(stderr, "Usage: %s %s\n", progname, arguments);
	if (description != NULL) {
		(void) fprintf(stderr, "\n%s\n", description);
	}

	if (freeFunction != NULL)
		(*freeFunction)();

	exit(exitcode);
}

void setup_signal_handler(const int signal, sighandler_t handler) {
	struct sigaction s;

	s.sa_handler = handler;
	s.sa_flags   = 0;
	if(sigfillset(&s.sa_mask) < 0) {
		bail_out(EXIT_FAILURE, "sigfillset");
	}
	if (sigaction(signal, &s, NULL) < 0) {
		bail_out(EXIT_FAILURE, "sigaction");
	}
}
