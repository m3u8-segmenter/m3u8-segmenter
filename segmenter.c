/*
 * Copyright (c) 2009 Chase Douglas
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

#include "libavformat/avformat.h"

static AVStream *add_output_stream(AVFormatContext *output_format_context, AVStream *input_stream) {
    AVCodecContext *input_codec_context;
    AVCodecContext *output_codec_context;
    AVStream *output_stream;

    output_stream = av_new_stream(output_format_context, 0);
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
        case CODEC_TYPE_AUDIO:
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
        case CODEC_TYPE_VIDEO:
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

int main(int argc, char **argv)
{
    const char *input;
    const char *output_prefix;
    double segment_duration;
    char *segment_duration_check;
    char *index;
    char *http_prefix;
    FILE *index_fp;
    double prev_segment_time = 0;
    unsigned int output_index = 1;
    char *write_buf;
    AVInputFormat *ifmt;
    AVOutputFormat *ofmt;
    AVFormatContext *ic = NULL;
    AVFormatContext *oc;
    AVStream *video_st;
    AVStream *audio_st;
    AVCodec *codec;
    char *output_filename;
    int video_index;
    int audio_index;
    int decode_done;
    int ret;
    int i;

    if (argc != 6) {
        fprintf(stderr, "Usage: %s <input MPEG-TS file> <segment duration in seconds> <output MPEG-TS file prefix> <output m3u8 index file> <http prefix>\n", argv[0]);
        exit(1);
    }

    av_register_all();

    input = argv[1];
    if (!strcmp(input, "-")) {
        input = "pipe:";
    }
    segment_duration = strtoll(argv[2], &segment_duration_check, 10);
    if (segment_duration_check == argv[2]) {
        fprintf(stderr, "Segment duration time (%s) invalid\n", argv[2]);
        exit(1);
    }
    output_prefix = argv[3];
    index = argv[4];
    http_prefix=argv[5];

    output_filename = malloc(sizeof(char) * (strlen(output_prefix) + 10));
    if (!output_filename) {
        fprintf(stderr, "Could not allocate space for output filenames\n");
        exit(1);
    }

    ifmt = av_find_input_format("mpegts");
    if (!ifmt) {
        fprintf(stderr, "Could not find MPEG-TS demuxer\n");
        exit(1);
    }

    ret = av_open_input_file(&ic, input, ifmt, 0, NULL);
    if (ret != 0) {
        fprintf(stderr, "Could not open input file, make sure it is an mpegts file: %d\n", ret);
        exit(1);
    }

    if (av_find_stream_info(ic) < 0) {
        fprintf(stderr, "Could not read stream information\n");
        exit(1);
    }

    ofmt = guess_format("mpegts", NULL, NULL);
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

    video_index = -1;
    audio_index = -1;

    for (i = 0; i < ic->nb_streams && (video_index < 0 || audio_index < 0); i++) {
        switch (ic->streams[i]->codec->codec_type) {
            case CODEC_TYPE_VIDEO:
                video_index = i;
                ic->streams[i]->discard = AVDISCARD_NONE;
                video_st = add_output_stream(oc, ic->streams[i]);
                break;
            case CODEC_TYPE_AUDIO:
                audio_index = i;
                ic->streams[i]->discard = AVDISCARD_NONE;
                audio_st = add_output_stream(oc, ic->streams[i]);
                break;
            default:
                ic->streams[i]->discard = AVDISCARD_ALL;
                break;
        }
    }

    if (av_set_parameters(oc, NULL) < 0) {
        fprintf(stderr, "Invalid output format parameters\n");
        exit(1);
    }

    dump_format(oc, 0, output_prefix, 1);

    codec = avcodec_find_decoder(video_st->codec->codec_id);
    if (!codec) {
        fprintf(stderr, "Could not find video decoder, key frames will not be honored\n");
    }

    if (avcodec_open(video_st->codec, codec) < 0) {
        fprintf(stderr, "Could not open video decoder, key frames will not be honored\n");
    }

    snprintf(output_filename, strlen(output_prefix) + 10, "%s-%05u.ts", output_prefix, output_index++);
    if (url_fopen(&oc->pb, output_filename, URL_WRONLY) < 0) {
        fprintf(stderr, "Could not open '%s'\n", output_filename);
        exit(1);
    }

    if (av_write_header(oc)) {
        fprintf(stderr, "Could not write mpegts header to first output file\n");
        exit(1);
    }

    index_fp = fopen(index, "w");
    write_buf = malloc(sizeof(char) * 1024);
    if (!index_fp) {
        fprintf(stderr, "Could not open m3u8 index file (%s), no index file will be created\n", index);
        if (write_buf) {
            free(write_buf);
            write_buf = NULL;
        }
    }
    else if (!write_buf) {
        fprintf(stderr, "Could not allocate write buffer for index file, index file will be invalid\n");
        fclose(index_fp);
        index_fp = NULL;
    }
    else {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%u\n", (unsigned int)segment_duration);
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
            fclose(index_fp);
            index_fp = NULL;
            free(write_buf);
            write_buf = NULL;
        }
    }

    do {
        double segment_time;
        AVPacket packet;

        decode_done = av_read_frame(ic, &packet);
        if (decode_done < 0) {
            break;
        }

        if (av_dup_packet(&packet) < 0) {
            fprintf(stderr, "Could not duplicate packet");
            av_free_packet(&packet);
            break;
        }

        if (packet.stream_index == video_index && (packet.flags & PKT_FLAG_KEY)) {
            segment_time = (double)video_st->pts.val * video_st->time_base.num / video_st->time_base.den;
        }
        else if (video_index < 0) {
            segment_time = (double)audio_st->pts.val * audio_st->time_base.num / audio_st->time_base.den;
        }
        else {
            segment_time = prev_segment_time;
        }

        if (segment_time - prev_segment_time >= segment_duration) {
            put_flush_packet(oc->pb);
            url_fclose(oc->pb);

            if (index_fp) {
                snprintf(write_buf, 1024, "#EXTINF:%u,\n%s%s\n", (unsigned int)segment_duration, http_prefix, output_filename);
                if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
                    fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
                    fclose(index_fp);
                    index_fp = NULL;
                    free(write_buf);
                    write_buf = NULL;
                }
                else {
                    fflush(index_fp);
                }
            }

            snprintf(output_filename, strlen(output_prefix) + 10, "%s-%05u.ts", output_prefix, output_index++);
            if (url_fopen(&oc->pb, output_filename, URL_WRONLY) < 0) {
                fprintf(stderr, "Could not open '%s'\n", output_filename);
                break;
            }

            prev_segment_time = segment_time;
        }

        ret = av_write_frame(oc, &packet);
        if (ret < 0) {
            fprintf(stderr, "Could not write frame of stream\n");
            av_free_packet(&packet);
            break;
        }
        else if (ret > 0) {
            fprintf(stderr, "End of stream requested\n");
            av_free_packet(&packet);
            break;
        }

        av_free_packet(&packet);
    } while (!decode_done);

    av_write_trailer(oc);

    avcodec_close(video_st->codec);

    for(i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    url_fclose(oc->pb);

    if (index_fp) {
        snprintf(write_buf, 1024, "#EXTINF:%u,\n%s%s\n#EXT-X-ENDLIST\n", (unsigned int)segment_duration, http_prefix, output_filename);
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write last file and endlist tag to m3u8 index file\n");
        }
    }

    free(output_filename);

    if (index_fp) {
        fclose(index_fp);
        free(write_buf);
    }

    av_free(oc);

    return 0;
}

// vim:sw=4:tw=4:ts=4:ai:expandtab
