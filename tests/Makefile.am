tests: test1 test2

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

ffmpeg_setup:
	mkdir -p ffmpeg
	if [ ! -f ffmpeg/ffmpeg-0.8.tar.bz2 ]; then cd ffmpeg && wget http://ffmpeg.org/releases/ffmpeg-0.8.tar.bz2 ; fi
	cd ffmpeg && tar xf ffmpeg-0.8.tar.bz2
	cd ffmpeg/ffmpeg-0.8 && ./configure \
		--disable-static \
		--enable-shared  \
		--enable-gpl \
		--disable-doc \
		--disable-ffplay \
		--disable-ffprobe \
		--disable-ffserver \
		--disable-avdevice \
		--disable-network \
		--disable-everything \
		--enable-encoder=mpeg2video \
		--enable-encoder=mp2 \
		--enable-decoder=theora \
		--enable-decoder=vorbis \
		--enable-muxer=mpeg2video \
		--enable-demuxer=ogg \
		--enable-filter=buffersink \
		--enable-libtheora \
		--enable-libvorbis
		make -C ffmpeg/ffmpeg-0.8

EXTRA_DIST = Hydrate-Kenny_Beltrey.oga big_buck_bunny.ogv test1.m3u8  test2.m3u8
