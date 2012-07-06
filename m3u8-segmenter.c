/*
 * Copyright (c) 2009 Chase Douglas
 * Copyright (c) 2011 John Ferlito
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <getopt.h>

#include <libavformat/avformat.h>
#include "libav-compat.h"

struct options_t {
    const char *input_file;
    long segment_duration;
    const char *output_prefix;
    const char *m3u8_file;
    char *tmp_m3u8_file;
    const char *url_prefix;
    long num_segments;
};


void handler(int signum);
static AVStream *add_output_stream(AVFormatContext *output_format_context, AVStream *input_stream);
int write_index_file(const struct options_t, const unsigned int first_segment, const unsigned int last_segment, const int end);
void display_usage(void);


int terminate = 0;


void handler(int signum) {
    (void)signum;
    terminate = 1;
}

static AVStream *add_output_stream(AVFormatContext *output_format_context, AVStream *input_stream) {
    AVCodecContext *input_codec_context;
    AVCodecContext *output_codec_context;
    AVStream *output_stream;

    output_stream = avformat_new_stream(output_format_context, 0);
    if (!output_stream) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }

    input_codec_context = input_stream->codec;
    output_codec_context = output_stream->codec;

    output_codec_context->codec_id = input_codec_context->codec_id;
    output_codec_context->codec_type = input_codec_context->codec_type;
    output_codec_context->codec_tag = input_codec_context->codec_tag;
    output_codec_context->bit_rate = input_codec_context->bit_rate;
    output_codec_context->extradata = input_codec_context->extradata;
    output_codec_context->extradata_size = input_codec_context->extradata_size;

    if(av_q2d(input_codec_context->time_base) * input_codec_context->ticks_per_frame > av_q2d(input_stream->time_base) && av_q2d(input_stream->time_base) < 1.0/1000) {
        output_codec_context->time_base = input_codec_context->time_base;
        output_codec_context->time_base.num *= input_codec_context->ticks_per_frame;
    }
    else {
        output_codec_context->time_base = input_stream->time_base;
    }

    switch (input_codec_context->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
            output_codec_context->channel_layout = input_codec_context->channel_layout;
            output_codec_context->sample_rate = input_codec_context->sample_rate;
            output_codec_context->channels = input_codec_context->channels;
            output_codec_context->frame_size = input_codec_context->frame_size;
            if ((input_codec_context->block_align == 1 && input_codec_context->codec_id == CODEC_ID_MP3) || input_codec_context->codec_id == CODEC_ID_AC3) {
                output_codec_context->block_align = 0;
            }
            else {
                output_codec_context->block_align = input_codec_context->block_align;
            }
            break;
        case AVMEDIA_TYPE_VIDEO:
            output_codec_context->pix_fmt = input_codec_context->pix_fmt;
            output_codec_context->width = input_codec_context->width;
            output_codec_context->height = input_codec_context->height;
            output_codec_context->has_b_frames = input_codec_context->has_b_frames;

            if (output_format_context->oformat->flags & AVFMT_GLOBALHEADER) {
                output_codec_context->flags |= CODEC_FLAG_GLOBAL_HEADER;
            }
            break;
    default:
        break;
    }

    return output_stream;
}

int write_index_file(const struct options_t options, const unsigned int first_segment, const unsigned int last_segment, const int end) {
    FILE *index_fp;
    char *write_buf;
    unsigned int i;

    index_fp = fopen(options.tmp_m3u8_file, "w");
    if (!index_fp) {
        fprintf(stderr, "Could not open temporary m3u8 index file (%s), no index file will be created\n", options.tmp_m3u8_file);
        return -1;
    }

    write_buf = malloc(sizeof(char) * 1024);
    if (!write_buf) {
        fprintf(stderr, "Could not allocate write buffer for index file, index file will be invalid\n");
        fclose(index_fp);
        return -1;
    }

    if (options.num_segments) {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%lu\n#EXT-X-MEDIA-SEQUENCE:%u\n", options.segment_duration, first_segment);
    }
    else {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%lu\n", options.segment_duration);
    }
    if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
        fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
        free(write_buf);
        fclose(index_fp);
        return -1;
    }

    for (i = first_segment; i <= last_segment; i++) {
        snprintf(write_buf, 1024, "#EXTINF:%lu,\n%s%s-%u.ts\n", options.segment_duration, options.url_prefix, options.output_prefix, i);
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
            free(write_buf);
            fclose(index_fp);
            return -1;
        }
    }

    if (end) {
        snprintf(write_buf, 1024, "#EXT-X-ENDLIST\n");
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write last file and endlist tag to m3u8 index file\n");
            free(write_buf);
            fclose(index_fp);
            return -1;
        }
    }

    free(write_buf);
    fclose(index_fp);

    return rename(options.tmp_m3u8_file, options.m3u8_file);
}

void display_usage(void)
{
    printf("Usage: m3u8-sementer [OPTION]...\n");
    printf("\n");
    printf("HTTP Live Streaming - Segments TS file and creates M3U8 index.");
    printf("\n");
    printf("\t-i, --input FILE             TS file to segment (Use - for stdin)\n");
    printf("\t-d, --duration SECONDS       Duration of each segment (default: 10 seconds)\n");
    printf("\t-p, --output-prefix PREFIX   Prefix for the TS segments, will be appended\n");
    printf("\t                             with -1.ts, -2.ts etc\n");
    printf("\t-m, --m3u8-file FILE         M3U8 output filename\n");
    printf("\t-u, --url-prefix PREFIX      Prefix for web address of segments, e.g. http://example.org/video/\n");
    printf("\t-n, --num-segment NUMBER     Number of segments to keep on disk\n");
    printf("\t-h, --help                   This help\n");
    printf("\n");
    printf("\n");

    exit(0);
}

int main(int argc, char **argv)
{
    double prev_segment_time = 0;
    unsigned int output_index = 1;
    AVInputFormat *ifmt;
    AVOutputFormat *ofmt;
    AVFormatContext *ic = NULL;
    AVFormatContext *oc;
    AVStream *video_st = NULL;
    AVStream *audio_st = NULL;
    AVCodec *codec;
    char *output_filename;
    char *remove_filename;
    int video_index = -1;
    int audio_index = -1;
    unsigned int first_segment = 1;
    unsigned int last_segment = 0;
    int write_index = 1;
    int decode_done;
    char *dot;
    int ret;
    unsigned int i;
    int remove_file;
    struct sigaction act;

    int opt;
    int longindex;
    char *endptr;
    struct options_t options;

    static const char *optstring = "i:d:p:m:u:n:ovh?";

    static const struct option longopts[] = {
        { "input",         required_argument, NULL, 'i' },
        { "duration",      required_argument, NULL, 'd' },
        { "output-prefix", required_argument, NULL, 'p' },
        { "m3u8-file",     required_argument, NULL, 'm' },
        { "url-prefix",    required_argument, NULL, 'u' },
        { "num-segments",  required_argument, NULL, 'n' },
        { "help",          no_argument,       NULL, 'h' },
        { 0, 0, 0, 0 }
    };


    memset(&options, 0 ,sizeof(options));

    /* Set some defaults */
    options.segment_duration = 10;
    options.num_segments = 0;

    do {
        opt = getopt_long(argc, argv, optstring, longopts, &longindex );
        switch (opt) {
            case 'i':
                options.input_file = optarg;
                if (!strcmp(options.input_file, "-")) {
                    options.input_file = "pipe:";
                }
                break;

            case 'd':
                options.segment_duration = strtol(optarg, &endptr, 10);
                if (optarg == endptr || options.segment_duration < 0 || options.segment_duration == -LONG_MAX) {
                    fprintf(stderr, "Segment duration time (%s) invalid\n", optarg);
                    exit(1);
                }
                break;

            case 'p':
                options.output_prefix = optarg;
                break;

            case 'm':
                options.m3u8_file = optarg;
                break;

            case 'u':
                options.url_prefix = optarg;
                break;

            case 'n':
                options.num_segments = strtol(optarg, &endptr, 10);
                if (optarg == endptr || options.num_segments < 0 || options.num_segments >= LONG_MAX) {
                    fprintf(stderr, "Maximum number of ts files (%s) invalid\n", optarg);
                    exit(1);
                }
                break;

            case 'h':
                display_usage();
                break;
        }
    } while (opt != -1);


    /* Check required args where set*/
    if (options.input_file == NULL) {
        fprintf(stderr, "Please specify an input file.\n");
        exit(1);
    }

    if (options.output_prefix == NULL) {
        fprintf(stderr, "Please specify an output prefix.\n");
        exit(1);
    }

    if (options.m3u8_file == NULL) {
        fprintf(stderr, "Please specify an m3u8 output file.\n");
        exit(1);
    }

    if (options.url_prefix == NULL) {
        fprintf(stderr, "Please specify a url prefix.\n");
        exit(1);
    }

    av_register_all();
    remove_filename = malloc(sizeof(char) * (strlen(options.output_prefix) + 15));
    if (!remove_filename) {
        fprintf(stderr, "Could not allocate space for remove filenames\n");
        exit(1);
    }

    output_filename = malloc(sizeof(char) * (strlen(options.output_prefix) + 15));
    if (!output_filename) {
        fprintf(stderr, "Could not allocate space for output filenames\n");
        exit(1);
    }

    options.tmp_m3u8_file = malloc(strlen(options.m3u8_file) + 2);
    if (!options.tmp_m3u8_file) {
        fprintf(stderr, "Could not allocate space for temporary index filename\n");
        exit(1);
    }

    // Use a dotfile as a temporary file
    strncpy(options.tmp_m3u8_file, options.m3u8_file, strlen(options.m3u8_file) + 2);
    dot = strrchr(options.tmp_m3u8_file, '/');
    dot = dot ? dot + 1 : options.tmp_m3u8_file;
    memmove(dot + 1, dot, strlen(dot));
    *dot = '.';

    ifmt = av_find_input_format("mpegts");
    if (!ifmt) {
        fprintf(stderr, "Could not find MPEG-TS demuxer\n");
        exit(1);
    }

    ret = avformat_open_input(&ic, options.input_file, ifmt, NULL);
    if (ret != 0) {
        fprintf(stderr, "Could not open input file, make sure it is an mpegts file: %d\n", ret);
        exit(1);
    }

    if (avformat_find_stream_info(ic, NULL) < 0) {
        fprintf(stderr, "Could not read stream information\n");
        exit(1);
    }

    ofmt = av_guess_format("mpegts", NULL, NULL);
    if (!ofmt) {
        fprintf(stderr, "Could not find MPEG-TS muxer\n");
        exit(1);
    }

    oc = avformat_alloc_context();
    if (!oc) {
        fprintf(stderr, "Could not allocated output context");
        exit(1);
    }
    oc->oformat = ofmt;

    for (i = 0; i < ic->nb_streams && (video_index < 0 || audio_index < 0); i++) {
        switch (ic->streams[i]->codec->codec_type) {
            case AVMEDIA_TYPE_VIDEO:
                video_index = i;
                ic->streams[i]->discard = AVDISCARD_NONE;
                video_st = add_output_stream(oc, ic->streams[i]);
                break;
            case AVMEDIA_TYPE_AUDIO:
                audio_index = i;
                ic->streams[i]->discard = AVDISCARD_NONE;
                audio_st = add_output_stream(oc, ic->streams[i]);
                break;
            default:
                ic->streams[i]->discard = AVDISCARD_ALL;
                break;
        }
    }

    // Don't print warnings when PTS and DTS are identical.
    ic->flags |= AVFMT_FLAG_IGNDTS;

    av_dump_format(oc, 0, options.output_prefix, 1);

    if (video_st) {
      codec = avcodec_find_decoder(video_st->codec->codec_id);
      if (!codec) {
          fprintf(stderr, "Could not find video decoder %x, key frames will not be honored\n", video_st->codec->codec_id);
      }

      if (avcodec_open2(video_st->codec, codec, NULL) < 0) {
          fprintf(stderr, "Could not open video decoder, key frames will not be honored\n");
      }
    }


    snprintf(output_filename, strlen(options.output_prefix) + 15, "%s-%u.ts", options.output_prefix, output_index++);
    if (avio_open(&oc->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
        fprintf(stderr, "Could not open '%s'\n", output_filename);
        exit(1);
    }

    if (avformat_write_header(oc, NULL)) {
        fprintf(stderr, "Could not write mpegts header to first output file\n");
        exit(1);
    }

    write_index = !write_index_file(options, first_segment, last_segment, 0);

    /* Setup signals */
    memset(&act, 0, sizeof(act));
    act.sa_handler = &handler;

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);

    do {
        double segment_time = prev_segment_time;
        AVPacket packet;

        if (terminate) {
          break;
        }

        decode_done = av_read_frame(ic, &packet);
        if (decode_done < 0) {
            break;
        }

        if (av_dup_packet(&packet) < 0) {
            fprintf(stderr, "Could not duplicate packet");
            av_free_packet(&packet);
            break;
        }

        // Use video stream as time base and split at keyframes. Otherwise use audio stream
        if (packet.stream_index == video_index && (packet.flags & AV_PKT_FLAG_KEY)) {
            segment_time = packet.pts * av_q2d(video_st->time_base);
        }
        else if (video_index < 0) {
            segment_time = packet.pts * av_q2d(audio_st->time_base);
        }
        else {
          segment_time = prev_segment_time;
        }


        if (segment_time - prev_segment_time >= options.segment_duration) {
            av_write_trailer(oc);   // close ts file and free memory
            avio_flush(oc->pb);
            avio_close(oc->pb);

            if (options.num_segments && (int)(last_segment - first_segment) >= options.num_segments - 1) {
                remove_file = 1;
                first_segment++;
            }
            else {
                remove_file = 0;
            }

            if (write_index) {
                write_index = !write_index_file(options, first_segment, ++last_segment, 0);
            }

            if (remove_file) {
                snprintf(remove_filename, strlen(options.output_prefix) + 15, "%s-%u.ts", options.output_prefix, first_segment - 1);
                remove(remove_filename);
            }

            snprintf(output_filename, strlen(options.output_prefix) + 15, "%s-%u.ts", options.output_prefix, output_index++);
            if (avio_open(&oc->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
                fprintf(stderr, "Could not open '%s'\n", output_filename);
                break;
            }

            // Write a new header at the start of each file
            if (avformat_write_header(oc, NULL)) {
              fprintf(stderr, "Could not write mpegts header to first output file\n");
              exit(1);
            }

            prev_segment_time = segment_time;
        }

        ret = av_interleaved_write_frame(oc, &packet);
        if (ret < 0) {
            fprintf(stderr, "Warning: Could not write frame of stream\n");
        }
        else if (ret > 0) {
            fprintf(stderr, "End of stream requested\n");
            av_free_packet(&packet);
            break;
        }

        av_free_packet(&packet);
    } while (!decode_done);

    av_write_trailer(oc);

    if (video_st) {
      avcodec_close(video_st->codec);
    }

    for(i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    avio_close(oc->pb);
    av_free(oc);

    if (options.num_segments && (int)(last_segment - first_segment) >= options.num_segments - 1) {
        remove_file = 1;
        first_segment++;
    }
    else {
        remove_file = 0;
    }

    if (write_index) {
        write_index_file(options, first_segment, ++last_segment, 1);
    }

    if (remove_file) {
        snprintf(remove_filename, strlen(options.output_prefix) + 15, "%s-%u.ts", options.output_prefix, first_segment - 1);
        remove(remove_filename);
    }

    return 0;
}
