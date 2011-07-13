CFLAGS=-g -O -Wall -Wstrict-prototypes -Wmissing-prototypes -Waggregate-return -Wcast-align -Wcast-qual -Wnested-externs -Wshadow -Wbad-function-cast -Wwrite-strings -Werror

all:
	gcc $(CFLAGS) m3u8-segmenter.c -o m3u8-segmenter -lavformat -lavcodec -lavutil

clean:
	rm -f m3u8-segmenter
	rm -f tests/tmp/*

install: m3u8-segmenter
	mkdir -p $(DESTDIR)/usr/bin
	cp m3u8-segmenter $(DESTDIR)/usr/bin
	mkdir -p $(DESTDIR)/usr/share/man/man1
	cp m3u8-segmenter.1 $(DESTDIR)/usr/share/man/man1
