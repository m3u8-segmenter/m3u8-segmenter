
/* Support older versions of ffmpeg and libav */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 64, 0)
  #define AVMEDIA_TYPE_AUDIO CODEC_TYPE_AUDIO
  #define AVMEDIA_TYPE_VIDEO CODEC_TYPE_VIDEO
  #define AV_PKT_FLAG_KEY PKT_FLAG_KEY
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(53, 2, 0)
  #define avformat_open_input(c,s,f,o) av_open_input_file(c,s,f,0,o)
  #define avformat_write_header(a, b) av_write_header(a)
  #define avio_open(...) url_fopen(__VA_ARGS__)
  #define avio_close(...) url_fclose(__VA_ARGS__)
  #define avio_flush(...) put_flush_packet(__VA_ARGS__)
  #define av_dump_format(...) dump_format(__VA_ARGS__)
#endif

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(52, 45, 0)
  #define av_guess_format(...) guess_format(__VA_ARGS__)
#endif
