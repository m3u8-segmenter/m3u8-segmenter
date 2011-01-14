CFLAGS=-g -O -Wall -Werror -pedantic -std=c99 -D_GNU_SOURCE

all:
	gcc $(CFLAGS) m3u8-segmenter.c -o m3u8-segmenter -lavformat -lavcodec -lavutil

clean:
	rm -f m3u8-segmenter

install: m3u8-segmenter
	mkdir -p $(DESTDIR)/usr/bin/
	cp m3u8-segmenter $(DESTDIR)/usr/bin/

