# Inactive and Deprecated

I no longer have much time to work on this project and for the most part it is deprecated
now that both ffmpeg and libav have direct support for segmenting and creating m3u files.
Quite happy to point this elsewhere if someone else would like to take over the project.

## m3u8-segmenter

**m3u8-segmenter** is an Apple HTTP Live Segmenter. It takes an MPEG-TS stream from a
tool like ffmpeg and breaks it up into multiple ts-segments suitable for HTTP
Live streaming from devices like iPhones. It also creates the M3U8 stream
descriptor which is required for this type of stream.

It was originally written by Chase Douglas and is now maintained by John
Ferlito.

##Installing

TO install from source you will need to make sure you have the libav/ffmpeg
libraries installed. On Ubuntu/Debian you would run

``` bash
apt-get install libavformat-dev
```

You can then build and install m3u8-segmenter by running

``` bash
aclocal
automake -ac
autoconf
./configure
make
sudo make install
```
## Usage

Some example command lines

``` bash
ffmpeg -loglevel quiet  -i big_buck_bunny.ogv -f mpegts - | \
  m3u8-segmenter -i - -d 10 -p tmp/big_buck_bunny -m tmp/big_buck.m3u8 -u http://inodes.org/bigbuck/

ffmpeg -er 4 -i input.mp3 -f mpegts -acodec libmp3lame -ar 22050 -ab 32k -vn - | \
  m3u8-segmenter -i - -d 10 -p outputdir/prefix -m outputdir/output.m3u8 -u http://domain.com/
```

## Note on Patches/Pull Requests

* Fork the project.
* Make your feature addition or bug fix.
* Add tests for it. This is important so I don't break it in a
  future version unintentionally.
* Commit, do not mess with Rakefile, version, or history.
  (if you want to have your own version, that is fine but bump version in a commit by itself I can ignore when I pull)
* Send me a pull request. Bonus points for topic branches.

Copyright
---------

Copyright (c) 2011 John Ferlito. See COPYING for details.
