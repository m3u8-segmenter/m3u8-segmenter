all:
	gcc -Wall -g segmenter.c -o segmenter -lavformat -lavcodec -lavutil

clean:
	rm -f segmenter

install: segmenter
	mkdir -p $(DESTDIR)/usr/bin/
	cp segmenter $(DESTDIR)/usr/bin/

