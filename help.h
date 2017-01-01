/**
 * @file help.h
 * @author overflowerror
 * @date 2016.11.27
 * @brief header file for the help functions
 * 
 * This file contains all definitions, constants, macros and prototypes for this lib.
 */

#ifndef HELP_H
#define HELP_H

/**
 * @brief count array entries
 *
 * This macro counts the entries in an array.
 */
#define COUNT(a) (sizeof(a)/sizeof(a[0]))

/**
 * @brief a signalhandler
 *
 * This is a type representing a signal handler.
 *
 * Appearently on my system signal.h doesn't define such a type.
 */
typedef void(*sighandler_t)(int);

/**
 * @brief a free-function
 *
 * This is a type representing a free-function, used to free resources on error.
 */
typedef void(*free_t)(void);

/**
 * @brief the program name
 *
 * The name of the progname, should initialized with help_init().
 */
extern const char* progname;

/**
 * @brief init this lib
 * @param function  the free-function; can be NULL
 * @param name      the name of the program
 *
 * Sets the given free-function and the program name.
 */
void help_init(free_t, const char*);

/**
 * @brief bail out from the program flow
 * @param exitcode      the code to exit with
 * @param module        the name of the current module; can be NULL
 * 
 * This function prints (to stderr) an error message (if the module name is not NULL),
 * prints strerror() (if errno is not 0), executes the free-function (if set), 
 * and exits the program with the given exit code.
 */
void bail_out(int, const char*);

/**
 * @brief displays a usage message
 * @param arguments     the string to display after the program name
 * @param description   a description of the arguments; can be NULL
 * @param error         a error message for a synopsis violation; can be NULL
 * @param exitcode      the code to exit with
 *
 * Prints to sterr:
 * - the error message (if given)
 * - the program name
 * - the arguments string
 * - the argument description (if given)
 * Then it exits with the given exit code.
 */
void usage(const char*, const char*, const char*, int);

/**
 * @brief sets up a signal handler
 * @param signal    the signal to set the handler up for
 * @param handler   the signal handler
 * 
 * This function sets up a signal handler for the given signal.
 */
void setup_signal_handler(const int, sighandler_t );

#endif
