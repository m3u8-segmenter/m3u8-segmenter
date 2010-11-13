all:
	gcc -Wall -g segmenter.c -o segmenter -lavformat -lavcodec -lavutil

clean:
	rm -f segmenter

install: segmenter
	mkdir $(DESTDIR)/bin/
	cp segmenter $(DESTDIR)/bin/

