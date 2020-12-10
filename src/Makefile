CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o net.o

all: ../server

../server: $(OBJS)
	$(CC) -o $@ $^

net.o: net.c net.h

server.o: server.c net.h

clean:
	rm -f $(OBJS)
	rm -f ../server

.PHONY: all, clean, tests
