# Changelog

## Unreleased

Features:

  - Move to using autoconf
  - Support newer version of ffmpeg (GH-8 help from Dominic Evans)
  - Improve the segmentation time
  - Add some more info to the README
  - Add all the options to the Makefile
  - Modify tests to use mp4 files

Bug Fixes:

  - Fix dashes in manpage.
  - Fix prefix option (GH-4 Bryan Murphy)
  - Remove unused ondemand option
  - Fix -n command line option (GH-11 Dominic Evans)
  - Ignore identical DTS and PTS warnings (GH-9 Justing Greer via Dominic Evans)
  - Improve unused parameter for signum (GH-13 Dominic Evans)

## 0.3 (25th June 2011)

Features:

  - Move to using real command line options
  - Support segmenting of audio files as well (#1)

Bug Fixes:

  - Support older version of libav (GH#2)
  - Make sure we write a header to the start of each file (GH-2).

## 0.2 (January 14, 2011)

Features:

  - John Ferlito took over as maintainer
  - Renamed the project to m3u8-segmenter
  - Added a man page for m3u8-segmenter
  - Added Debian packaging meta data

Bugfixes:

  - Make sure EXT-X-ENDLIST is written to the file on exit


## 0.1 (Never released)

Previous release by Chase Douglas

