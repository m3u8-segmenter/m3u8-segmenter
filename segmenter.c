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

int write_index_file(const char index[], const char tmp_index[], const unsigned int segment_duration, const char output_prefix[], const char http_prefix[], const unsigned int first_segment, const unsigned int last_segment, const int end, const int window) {
    FILE *index_fp;
    char *write_buf;
    unsigned int i;

    index_fp = fopen(tmp_index, "w");
    if (!index_fp) {
        fprintf(stderr, "Could not open temporary m3u8 index file (%s), no index file will be created\n", tmp_index);
        return -1;
    }

    write_buf = malloc(sizeof(char) * 1024);
    if (!write_buf) {
        fprintf(stderr, "Could not allocate write buffer for index file, index file will be invalid\n");
        fclose(index_fp);
        return -1;
    }

    if (window) {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%u\n#EXT-X-MEDIA-SEQUENCE:%u\n", segment_duration, first_segment);
    }
    else {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-TARGETDURATION:%u\n", segment_duration);
    }
    if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
        fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
        free(write_buf);
        fclose(index_fp);
        return -1;
    }

    for (i = first_segment; i <= last_segment; i++) {
        snprintf(write_buf, 1024, "#EXTINF:%u,\n%s%s-%u.ts\n", segment_duration, http_prefix, output_prefix, i);
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

    return rename(tmp_index, index);
}

int main(int argc, char **argv)
{
    const char *input;
    const char *output_prefix;
    double segment_duration;
    char *segment_duration_check;
    const char *index;
    char *tmp_index;
    const char *http_prefix;
    long max_tsfiles = 0;
    char *max_tsfiles_check;
    double prev_segment_time = 0;
    unsigned int output_index = 1;
    AVInputFormat *ifmt;
    AVOutputFormat *ofmt;
    AVFormatContext *ic = NULL;
    AVFormatContext *oc;
    AVStream *video_st;
    AVStream *audio_st;
    AVCodec *codec;
    char *output_filename;
    char *remove_filename;
    int video_index;
    int audio_index;
    unsigned int first_segment = 1;
    unsigned int last_segment = 0;
    int write_index = 1;
    int decode_done;
    char *dot;
    int ret;
    int i;
    int remove_file;

    if (argc < 6 || argc > 7) {
        fprintf(stderr, "Usage: %s <input MPEG-TS file> <segment duration in seconds> <output MPEG-TS file prefix> <output m3u8 index file> <http prefix> [<segment window size>]\n", argv[0]);
        exit(1);
    }

    av_register_all();

    input = argv[1];
    if (!strcmp(input, "-")) {
        input = "pipe:";
    }
    segment_duration = strtod(argv[2], &segment_duration_check);
    if (segment_duration_check == argv[2] || segment_duration == HUGE_VAL || segment_duration == -HUGE_VAL) {
        fprintf(stderr, "Segment duration time (%s) invalid\n", argv[2]);
        exit(1);
    }
    output_prefix = argv[3];
    index = argv[4];
    http_prefix=argv[5];
    if (argc == 7) {
        max_tsfiles = strtol(argv[6], &max_tsfiles_check, 10);
        if (max_tsfiles_check == argv[6] || max_tsfiles < 0 || max_tsfiles >= INT_MAX) {
            fprintf(stderr, "Maximum number of ts files (%s) invalid\n", argv[6]);
            exit(1);
        }
    }

    remove_filename = malloc(sizeof(char) * (strlen(output_prefix) + 15));
    if (!remove_filename) {
        fprintf(stderr, "Could not allocate space for remove filenames\n");
        exit(1);
    }

    output_filename = malloc(sizeof(char) * (strlen(output_prefix) + 15));
    if (!output_filename) {
        fprintf(stderr, "Could not allocate space for output filenames\n");
        exit(1);
    }

    tmp_index = malloc(strlen(index) + 2);
    if (!tmp_index) {
        fprintf(stderr, "Could not allocate space for temporary index filename\n");
        exit(1);
    }

    strncpy(tmp_index, index, strlen(index) + 2);
    dot = strrchr(tmp_index, '/');
    dot = dot ? dot + 1 : tmp_index;
    for (i = strlen(tmp_index) + 1; i > dot - tmp_index; i--) {
        tmp_index[i] = tmp_index[i - 1];
    }
    *dot = '.';

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

    snprintf(output_filename, strlen(output_prefix) + 15, "%s-%u.ts", output_prefix, output_index++);
    if (url_fopen(&oc->pb, output_filename, URL_WRONLY) < 0) {
        fprintf(stderr, "Could not open '%s'\n", output_filename);
        exit(1);
    }

    if (av_write_header(oc)) {
        fprintf(stderr, "Could not write mpegts header to first output file\n");
        exit(1);
    }

    write_index = !write_index_file(index, tmp_index, segment_duration, output_prefix, http_prefix, first_segment, last_segment, 0, max_tsfiles);

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

            if (max_tsfiles && (int)(last_segment - first_segment) >= max_tsfiles - 1) {
                remove_file = 1;
                first_segment++;
            }
            else {
                remove_file = 0;
            }

            if (write_index) {
                write_index = !write_index_file(index, tmp_index, segment_duration, output_prefix, http_prefix, first_segment, ++last_segment, 0, max_tsfiles);
            }

            if (remove_file) {
                snprintf(remove_filename, strlen(output_prefix) + 15, "%s-%u.ts", output_prefix, first_segment - 1);
                remove(remove_filename);
            }

            snprintf(output_filename, strlen(output_prefix) + 15, "%s-%u.ts", output_prefix, output_index++);
            if (url_fopen(&oc->pb, output_filename, URL_WRONLY) < 0) {
                fprintf(stderr, "Could not open '%s'\n", output_filename);
                break;
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

    avcodec_close(video_st->codec);

    for(i = 0; i < oc->nb_streams; i++) {
        av_freep(&oc->streams[i]->codec);
        av_freep(&oc->streams[i]);
    }

    url_fclose(oc->pb);
    av_free(oc);

    if (max_tsfiles && (int)(last_segment - first_segment) >= max_tsfiles - 1) {
        remove_file = 1;
        first_segment++;
    }
    else {
        remove_file = 0;
    }

    if (write_index) {
        write_index_file(index, tmp_index, segment_duration, output_prefix, http_prefix, first_segment, ++last_segment, 1, max_tsfiles);
    }

    if (remove_file) {
        snprintf(remove_filename, strlen(output_prefix) + 15, "%s-%u.ts", output_prefix, first_segment - 1);
        remove(remove_filename);
    }

    return 0;
}

// vim:sw=4:tw=4:ts=4:ai:expandtab
