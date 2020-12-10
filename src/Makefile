CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o net.o http.o file.o

all: ../server

../server: $(OBJS)
	$(CC) -o $@ $^

net.o: net.c net.h

file.o: file.c file.h

http.o: http.c http.h file.h

server.o: server.c net.h http.h

clean:
	rm -f $(OBJS)
	rm -f ../server

.PHONY: all, clean, tests
