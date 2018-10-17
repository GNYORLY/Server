CC=gcc
CPPFLAGS=-g -Wall
USERID=504666652

all: server

server:
	$(CC) -o $@ $^ $(CPPFLAGS) $@.c

clean:
	rm -rf *.dSYM server *.tar.gz

dist: 
	tar -cvzf $(USERID).tar.gz server.c Makefile README report.pdf 404.html
