.PHONY: clean

CFLAGS=-Wall -pedantic -std=gnu99 -g

all: server client

server.o: server.c copie.h
client.o: client.c copie.h

server: server.o copie.o

client: client.o copie.o

clean:
	rm -rf server client
	rm -f *~ *.o
