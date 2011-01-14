all: test1

FFMPEG=ffmpeg -loglevel quiet  -i big_buck_bunny.ogg -f mpegts -

test1: preparation
	$(FFMPEG) 2>/dev/null | ../m3u8-segmenter -i - -d 10 -p tmp/big_buck_bunny -m tmp/big_buck.m3u8 -u http://inodes.org/bigbuck/
	diff test1.m3u8 tmp/big_buck.m3u8

preparation:
	rm -f tmp/*
