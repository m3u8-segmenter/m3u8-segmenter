all: test1 test2

FFMPEG_VIDEO=ffmpeg -loglevel quiet  -i big_buck_bunny.ogv -f mpegts -
FFMPEG_AUDIO=ffmpeg -loglevel quiet  -i Hydrate-Kenny_Beltrey.oga -f mpegts -

test1: preparation
	$(FFMPEG_VIDEO) 2>/dev/null | ../m3u8-segmenter -i - -d 10 -p tmp/big_buck_bunny -m tmp/big_buck.m3u8 -u http://inodes.org/bigbuck/
	diff test1.m3u8 tmp/big_buck.m3u8

test2: preparation
	$(FFMPEG_AUDIO) 2>/dev/null | ../m3u8-segmenter -i - -d 10 -p tmp/hydrate -m tmp/hydrate.m3u8 -u http://inodes.org/hydrate/
	diff test2.m3u8 tmp/hydrate.m3u8

preparation:
	rm -f tmp/*
