CC=gcc
CFLAGS=-std=gnu11 -lkcgi -lz -lm -lopc -I/usr/include/libxml2/ -lmce -lxml2 -flto -O2
DEPS = grouptext.h grouptext.c json.c json.h

all: textscore scorefile scoreurl

textscore: textscore
	$(CC) -o textscore.fcgi scoretext.c json.c grouptext.c $(CFLAGS)

scorefile: scorefile
	$(CC) -o scorefile.fcgi scorefile.c json.c grouptext.c $(CFLAGS)

scoreurl: scoreurl
	$(CC) -o scoreurl.fcgi scoreurl.c json.c grouptext.c $(CFLAGS)
