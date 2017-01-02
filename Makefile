#
# Makefile for Serwer
#
# Author: overflowerror
#
#

CC = gcc
DEFS = -D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS = -Wall -Wextra -g -std=c99 -pedantic -DENDEBUG $(DEFS)
LDFLAGS = $(DEFS)

.PHONY: all clean

all: example

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

example: example.o webserver.o help.o
	$(CC) $(LDFLAGS) -o $@ $^

example.o: example.c webserver.h

help.o: help.c help.h

webserver.o: webserver.c webserver.h help.h

clean: 
	rm -f *.o example
