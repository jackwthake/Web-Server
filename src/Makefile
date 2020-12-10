CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o http.o util/file.o util/net.o 

all: ../server

../server: $(OBJS)
	$(CC) -o $@ $^

util/net.o: util/net.c util/net.h

util/file.o: util/file.c util/file.h

http.o: http.c http.h util/file.h

server.o: server.c util/net.h http.h

clean:
	rm -f $(OBJS)
	rm -f ../server

.PHONY: all, clean, tests
