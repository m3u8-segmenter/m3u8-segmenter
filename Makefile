all:
	gcc -Wall -g segmenter.c -o segmenter -lavformat -lavcodec -lavutil

clean:
	rm segmenter
