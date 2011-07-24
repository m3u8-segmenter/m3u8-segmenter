
# Don't care about NEWS file etc
AUTOMAKE_OPTIONS = foreign

SUBDIRS = tests

bin_PROGRAMS = m3u8-segmenter

m3u8_segmenter_SOURCES = m3u8-segmenter.c
m3u8_segmenter_CFLAGS = $(FFMPEG_CFLAGS)
m3u8_segmenter_LDADD = $(FFMPEG_LIBS)

dist_man_MANS = m3u8-segmenter.1

EXTRA_DIST = CHANGELOG.md README.md
