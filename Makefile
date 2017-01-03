#
# Makefile for Serwer
#
# Author: overflowerror
#
#

CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic -DENDEBUG $(DEFS) -fPIC
LDFLAGS = $(DEFS)
LIBFLAGS = -shared $(DEFS)

.PHONY: all clean

all: example libserwer.so

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

example: example.o serwer.o ws_modes/linear.o ws_handlers/info.o ws_utils.o help.o 
	$(CC) $(LDFLAGS) -o $@ $^

libserwer.so: serwer.o ws_modes/linear.o ws_handlers/info.o ws_utils.o help.o
	$(CC) $(LIBFLAGS) -o $@ $^

example.o: example.c serwer.h help.h
help.o: help.c help.h
serwer.o: serwer.c serwer.h ws_types.h ws_modes.h ws_utils.h help.h
ws_utils.o: ws_utils.c ws_utils.h ws_types.h 
ws_modes/linear.o: ws_modes/linear.c ws_modes.h ws_types.h serwer.h ws_utils.h
ws_handlers/info.o: ws_handlers/info.c ws_handlers.h serwer.h

clean: 
	rm -f *.o ws_modes/*.o example libserwer.so
