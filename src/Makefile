CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o net.o http.o

all: ../server

../server: $(OBJS)
	$(CC) -o $@ $^

net.o: net.c net.h

http.o: http.c http.h

server.o: server.c net.h http.h

clean:
	rm -f $(OBJS)
	rm -f ../server

.PHONY: all, clean, tests
