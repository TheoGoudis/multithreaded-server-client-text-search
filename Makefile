CC = gcc
CFLAGS = -g -O2 -Wall -Wundef
OBJECTS = 

all: client server

client: client.c utils.o
	$(CC) $(CFLAGS) -o client client.c utils.o

server: server.c utils.o
	$(CC) $(CFLAGS) -o server server.c utils.o -lpthread

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o client server

