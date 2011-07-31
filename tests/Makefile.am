EXTRA_DIST = Hydrate-Kenny_Beltrey.oga big_buck_bunny.ogv test1.m3u8  test2.m3u8

tests: libav ffmpeg

FFMPEG_CONFIGURE = --disable-static \
   --enable-shared  \
   --enable-gpl \
   --disable-ffprobe \
   --disable-ffserver \
   --disable-doc \
   --disable-avdevice \
   --disable-network \
   --disable-everything \
   --enable-protocol=file \
   --enable-protocol=pipe \
   \
   --enable-encoder=libmp3lame \
   --enable-encoder=libx264 \
   \
   --enable-decoder=theora \
   --enable-decoder=vorbis \
   --enable-decoder=flac \
   --enable-decoder=h264 \
   --enable-decoder=mp3 \
   \
   --enable-muxer=mpegts \
   --enable-muxer=mp3 \
   \
   --enable-demuxer=ogg \
   --enable-demuxer=mpegts \
   \
   --enable-filter=buffersink \
   --enable-filter=buffer \
   --enable-filter=scale \
   \
   --enable-libtheora \
   --enable-libvorbis \
   --enable-libmp3lame \
   --enable-libx264

libav:  get_libav compile_libav test_libav
ffmpeg: get_ffmpeg compile_ffmpeg test_ffmpeg
local: compile_local test_local

libav_vars:
	$(eval LIB=libav)
	$(eval LIB_VERSION=0.7.1)
	$(eval PREFIX=$(shell pwd)/$(LIB)/$(LIB)-$(LIB_VERSION)/install)
	$(eval FFMPEG_PATH=LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin)

ffmpeg_vars:
	$(eval LIB=ffmpeg)
	$(eval LIB_VERSION=0.8)
	$(eval PREFIX=$(shell pwd)/$(LIB)/$(LIB)-$(LIB_VERSION)/install)
	$(eval FFMPEG_PATH=LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin)

local_vars:
	mkdir -p local
	$(eval PREFIX=$(shell pwd)/local)
	$(eval FFMPEG_PATH="/usr/bin")


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


FFMPEG_OPTIONS=-f mpegts -acodec libmp3lame -ar 48000 -ab 64k -s 320x240 -vcodec libx264 -b 96k -flags +loop -cmp +chroma -partitions +parti4x4+partp8x8+partb8x8 -subq 5 -trellis 1 -refs 1 -coder 0 -me_range 16 -keyint_min 25 -sc_threshold 40 -i_qfactor 0.71 -bt 200k -maxrate 96k -bufsize 96k -rc_eq 'blurCplx^(1-qComp)' -qcomp 0.6 -qmin 10 -qmax 51 -qdiff 4 -level 30 -aspect 320:240 -g 30 -async 2

video_test:
	$(FFMPEG_PATH)/ffmpeg -i big_buck_bunny.ogv $(FFMPEG_OPTIONS) - 2>/dev/null | LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin/m3u8-segmenter -i - -d 10 -p tmp/big_buck_bunny -m tmp/big_buck.m3u8 -u http://inodes.org/bigbuck/
	diff -u test1.m3u8 tmp/big_buck.m3u8
	for i in tmp/big_buck_bunny-[0-9]*.ts; do $(FFMPEG_PATH)/ffmpeg -i $$i 2>&1 | grep Duration; done

audio_test:
	$(FFMPEG_PATH)/ffmpeg -i Hydrate-Kenny_Beltrey.oga $(FFMPEG_OPTIONS) - 2>/dev/null | LD_LIBRARY_PATH=$(PREFIX)/lib $(PREFIX)/bin/m3u8-segmenter -i - -d 10 -p tmp/hydrate -m tmp/hydrate.m3u8 -u http://inodes.org/hydrate/
	diff -u test2.m3u8 tmp/hydrate.m3u8
	for i in tmp/hydrate-[0-9]*.ts; do $(FFMPEG_PATH)/ffmpeg -i $$i 2>&1 | grep Duration; done

preparation:
	rm -f tmp/*

