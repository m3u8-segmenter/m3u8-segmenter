EXTRA_DIST = Hydrate-Kenny_Beltrey.oga big_buck_bunny.ogv test1.m3u8  test2.m3u8

FFMPEG_OPTIONS=-f mpegts -acodec libmp3lame -ar 48000 -ab 64k -s 320x240 -vcodec libx264 -b 96k -flags +loop -cmp +chroma -partitions +parti4x4+partp8x8+partb8x8 -subq 5 -trellis 1 -refs 1 -coder 0 -me_range 16 -keyint_min 25 -sc_threshold 40 -i_qfactor 0.71 -bt 200k -maxrate 96k -bufsize 96k -rc_eq 'blurCplx^(1-qComp)' -qcomp 0.6 -qmin 10 -qmax 51 -qdiff 4 -level 30 -aspect 320:240 -g 30 -async 2

video_test:
	ffmpeg -i big_buck_bunny.ogv $(FFMPEG_OPTIONS) - 2>/dev/null | ../m3u8-segmenter -i - -d 10 -p tmp/big_buck_bunny -m tmp/big_buck.m3u8 -u http://inodes.org/bigbuck/
	diff -u test1.m3u8 tmp/big_buck.m3u8
	for i in tmp/big_buck_bunny-[0-9]*.ts; do ffmpeg -i $$i 2>&1 | grep Duration; done

audio_test:
	ffmpeg -i Hydrate-Kenny_Beltrey.oga $(FFMPEG_OPTIONS) - 2>/dev/null | ../m3u8-segmenter -i - -d 10 -p tmp/hydrate -m tmp/hydrate.m3u8 -u http://inodes.org/hydrate/
	diff -u test2.m3u8 tmp/hydrate.m3u8
	for i in tmp/hydrate-[0-9]*.ts; do ffmpeg -i $$i 2>&1 | grep Duration; done

preparation:
	rm -f tmp/*

