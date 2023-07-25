CC=gcc
LDFLAGS=-lm -pthread
CFLAGS=-g -pedantic -std=gnu17 -fsigned-char

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o 

nyuenc.o: nyuenc.c 

.PHONY: clean
clean:
	rm -f *.o nyuenc

