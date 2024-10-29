# Makefile for Proxy Lab and Echo Client/Server
CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

# Target to build all executables
all: proxy echo_client echo_server

# Object files for csapp library
csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

# Object files for proxy
proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

# Object files for echo_client
echo_client.o: echo_client.c csapp.h
	$(CC) $(CFLAGS) -c echo_client.c

# Object files for echo_server
echo_server.o: echo_server.c csapp.h
	$(CC) $(CFLAGS) -c echo_server.c

# Build executables
proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

echo_client: echo_client.o csapp.o
	$(CC) $(CFLAGS) echo_client.o csapp.o -o echo_client $(LDFLAGS)

echo_server: echo_server.o csapp.o
	$(CC) $(CFLAGS) echo_server.o csapp.o -o echo_server $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude echo --exclude echo_client --exclude echo_server --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

# Clean up object files and executables
clean:
	rm -f *~ *.o proxy echo_client echo_server core *.tar *.zip *.gzip *.bzip *.gz
