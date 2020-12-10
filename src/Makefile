CC=gcc
CFLAGS=-Wall -Wextra

OBJS=server.o http.o mime.o util/file.o util/net.o 

all: ../server

../server: $(OBJS)
	$(CC) -o $@ $^

util/net.o: util/net.c util/net.h

util/file.o: util/file.c util/file.h

mime.o: mime.c mime.h

http.o: http.c http.h util/file.h

server.o: server.c http.h mime.h util/net.h

clean:
	rm -f $(OBJS)
	rm -f ../server

.PHONY: all, clean, tests
