EXTRA_DIST = Hydrate-Kenny_Beltrey.oga big_buck_bunny.ogv test1.m3u8  test2.m3u8

tests: libav ffmpeg

FFMPEG_CONFIGURE = --disable-static \
   --enable-shared  \
   --enable-gpl \
   --disable-ffplay \
   --disable-ffprobe \
   --disable-ffserver \
   --disable-doc \
   --disable-avdevice \
   --disable-network \
   --disable-everything \
   --enable-protocol=file \
   --enable-protocol=pipe \
   --enable-encoder=mpeg2video \
   --enable-encoder=mp2 \
   --enable-encoder=mp3 \
   --enable-decoder=theora \
   --enable-decoder=vorbis \
   --enable-decoder=flac \
   --enable-decoder=mp2 \
   --enable-decoder=mp3 \
   --enable-decoder=mpeg2video \
   --enable-muxer=mpeg2video \
   --enable-muxer=mpegts \
   --enable-muxer=mp3 \
   --enable-demuxer=ogg \
   --enable-demuxer=mpegts \
   --enable-demuxer=mp3 \
   --enable-filter=buffersink \
   --enable-libtheora \
   --enable-libvorbis

libav:  get_libav compile_libav test_libav
ffmpeg: get_ffmpeg compile_ffmpeg test_ffmpeg
local: compile_local test_local

libav_vars:
	$(eval LIB=libav)
	$(eval LIB_VERSION=0.7.1)
	$(eval PREFIX=$(shell pwd)/$(LIB)/$(LIB)-$(LIB_VERSION)/install)

ffmpeg_vars:
	$(eval LIB=ffmpeg)
	$(eval LIB_VERSION=0.8)
	$(eval PREFIX=$(shell pwd)/$(LIB)/$(LIB)-$(LIB_VERSION)/install)

local_vars:
	mkdir -p local
	$(eval PREFIX=$(shell pwd)/local)



get_libav: libav_vars get
compile_libav: libav_vars compile
test_libav: libav_vars video_test audio_test

get_ffmpeg: ffmpeg_vars get
compile_ffmpeg: ffmpeg_vars compile
test_ffmpeg: ffmpeg_vars video_test audio_test

compile_local: local_vars compile
test_local: local_vars video_test audio_test

get:
	mkdir -p $(LIB)
	if [ ! -f $(LIB)/$(LIB)-$(LIB_VERSION).tar.gz ]; then cd $(LIB) && wget http://$(LIB).org/releases/$(LIB)-$(LIB_VERSION).tar.gz;  fi
	cd $(LIB) && tar xf $(LIB)-$(LIB_VERSION).tar.gz
	cd $(LIB)/$(LIB)-$(LIB_VERSION) && ./configure \
		$(FFMPEG_CONFIGURE) \
		--prefix=$(PREFIX)
	make -C $(LIB)/$(LIB)-$(LIB_VERSION) install

compile:
	cd .. && PKG_CONFIG_PATH=$(PREFIX)/lib/pkgconfig ./configure --prefix=$(PREFIX)
	cd .. && make clean
	cd .. && make install



FFMPEG_VIDEO=ffmpeg -loglevel quiet  -i big_buck_bunny.ogv -f mpegts -
FFMPEG_AUDIO=ffmpeg -loglevel quiet  -i Hydrate-Kenny_Beltrey.oga -f mpegts -

video_test:
	LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin/$(FFMPEG_VIDEO) 2>/dev/null | LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin/m3u8-segmenter -i - -d 10 -p tmp/big_buck_bunny -m tmp/big_buck.m3u8 -u http://inodes.org/bigbuck/
	diff test1.m3u8 tmp/big_buck.m3u8
	for i in tmp/big_buck_bunny-[0-9]*.ts; do


audio_test:
	LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin/$(FFMPEG_AUDIO) 2>/dev/null | LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin/m3u8-segmenter -i - -d 10 -p tmp/hydrate -m tmp/hydrate.m3u8 -u http://inodes.org/hydrate/
	diff test2.m3u8 tmp/hydrate.m3u8

preparation:
	rm -f tmp/*

