CC=gcc
CFLAGS=-s -std=gnu11 -lkcgi -lz -lm -lopc -I/usr/include/libxml2/ -lmce -lxml2 -lpthread -flto -O2
DEPS = grouptext.h grouptext.c json.c json.h pool.h

all: textscore scorefile scoreurl

textscore:
	$(CC) -o textscore.fcgi scoretext.c json.c grouptext.c $(CFLAGS)

scorefile:
	$(CC) -o scorefile.fcgi scorefile.c json.c grouptext.c $(CFLAGS)

scoreurl:
	$(CC) -o scoreurl.fcgi scoreurl.c json.c grouptext.c $(CFLAGS)
