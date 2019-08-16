CC=gcc
CFLAGS=-s -std=gnu11 -lkcgi -lz -lm -lopc -I/usr/include/libxml2/ -lmce -lxml2 -lpthread -lmagic -flto -O2
DEPS = grouptext.h grouptext.c json.c json.h pool.h base85.c base85.h

all: textscore scorefile scoreurl

textscore:
	$(CC) -o textscore.fcgi scoretext.c $(DEPS) $(CFLAGS)

scorefile:
	$(CC) -o scorefile.fcgi scorefile.c $(DEPS) $(CFLAGS)

scoreurl:
	$(CC) -o scoreurl.fcgi scoreurl.c $(DEPS) $(CFLAGS)
