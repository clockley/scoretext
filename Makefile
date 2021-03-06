CC=gcc
CFLAGS=-s -std=gnu11 -lkcgi -lz -lm -lopc -I/usr/include/libxml2/ -lmce -lxml2 -lpthread -lmagic -lcurl -ldfp -lJudy -flto -O2
DEPS = textutils.h textutils.c json.c json.h pool.h base64.c base64.h pdf.h pdf.c

all: textscore scorefile scoreurl

textscore:
	$(CC) -o textscore.fcgi scoretext.c $(DEPS) $(CFLAGS)

scorefile:
	$(CC) -o scorefile.fcgi scorefile.c $(DEPS) $(CFLAGS)

scoreurl:
	$(CC) -o scoreurl.fcgi scoreurl.c $(DEPS) $(CFLAGS)
