CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread

all: proxy echo  # proxy와 echo 두 개의 타겟을 빌드

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

echo.o: echo.c csapp.h  # echo.c도 포함
	$(CC) $(CFLAGS) -c echo.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

echo: echo.o csapp.o  # echo 타겟 빌드
	$(CC) $(CFLAGS) echo.o csapp.o -o echo $(LDFLAGS)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude echo --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy echo core *.tar *.zip *.gzip *.bzip *.gz
