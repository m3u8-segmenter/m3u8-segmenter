all:
	gcc -Wall -g m3u8-segmenter.c -o m3u8-segmenter -lavformat -lavcodec -lavutil

clean:
	rm -f m3u8-segmenter

install: m3u8-segmenter
	mkdir -p $(DESTDIR)/usr/bin/
	cp m3u8-segmenter $(DESTDIR)/usr/bin/

