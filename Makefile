CC= /usr/bin/gcc

all:	udpclient udpserver

udpclient: udpclient.c;
	${CC} udpclient.c -g -o udpclient

udpserver: udpserver.c;
	${CC} udpserver.c -g -o udpserver

clean:
	rm udpclient udpserver
