/*
 * video_rec.c
 *
 *  Created on: Mar 16, 2011
 *      Author: bh
 *
 *  This file is part of GLMixer.
 *
 *   GLMixer is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   GLMixer is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with GLMixer.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Copyright 2009, 2010 Bruno Herbelin
 *
 */

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>

#include "video_rec.h"
#include <stdio.h>

struct encoder {
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVCodecContext *v_ctx;
	AVStream *v_st;
	void *vbuf_ptr;
	size_t vbuf_size;
};

struct converter {
	AVFrame *picture;
	uint8_t *picture_buf;
	struct SwsContext *img_convert_ctx;
};



video_rec_t *
video_rec_init(const char *filename, encodingformat f, int width, int height, int fps, char *errormessage)
{
	AVCodec *c = NULL;
    char buf[1024];

	video_rec_t *rec = calloc(1, sizeof(video_rec_t));
	rec->enc = calloc(1, sizeof(struct encoder));

	// fill in basic info
	rec->width = width;
	rec->height = height;
	rec->fps = fps;
	rec->framenum = 0;

	// setup according to format
	char f_name[9] = "";
	enum CodecID f_codec_id = CODEC_ID_NONE;
	enum PixelFormat f_pix_fmt =  PIX_FMT_NONE;
	switch (f){
	case FORMAT_MPG_MPEG1:
		snprintf(f_name, 9, "mpeg");
		f_codec_id = CODEC_ID_MPEG1VIDEO;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &sws_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		snprintf(rec->suffix, 6, "mpg");
		snprintf(rec->description, 64, "MPEG Video (*.mpg *.mpeg)");
		break;
	case FORMAT_WMV_WMV2:
		snprintf(f_name, 9, "avi");
		f_codec_id = CODEC_ID_WMV2;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &sws_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		snprintf(rec->suffix, 6, "wmv");
		snprintf(rec->description, 64, "Windows Media Video (*.wmv)");
		break;
	case FORMAT_FLV_FLV1:
		snprintf(f_name, 9, "flv");
		f_codec_id = CODEC_ID_FLV1;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &sws_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		snprintf(rec->suffix, 6, "flv");
		snprintf(rec->description, 64, "Flash Video (*.flv)");
		break;
	case FORMAT_MP4_MPEG4:
		snprintf(f_name, 9, "mp4");
		f_codec_id = CODEC_ID_MPEG4;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &sws_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		snprintf(rec->suffix, 6, "mp4");
		snprintf(rec->description, 64, "MPEG 4 Video (*.mp4)");
		break;
	case FORMAT_MPG_MPEG2:
		snprintf(f_name, 9, "mpeg");
		f_codec_id = CODEC_ID_MPEG2VIDEO;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &sws_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		snprintf(rec->suffix, 6, "mpg");
		snprintf(rec->description, 64, "MPEG Video (*.mpg *.mpeg)");
		break;
	case FORMAT_AVI_FFVHUFF:
		snprintf(f_name, 9, "avi");
		f_codec_id = CODEC_ID_FFVHUFF;
		f_pix_fmt =  PIX_FMT_BGRA;
		rec->pt2RecordingFunction = &rec_deliver_vframe;
		rec->conv = NULL;
		snprintf(rec->suffix, 6, "avi");
		snprintf(rec->description, 64, "AVI Video (*.avi)");
		break;
	default:
	case FORMAT_AVI_RAW:
		snprintf(f_name, 9, "avi");
		f_codec_id = CODEC_ID_RAWVIDEO;
		f_pix_fmt =  PIX_FMT_BGRA;
		rec->pt2RecordingFunction = &rec_deliver_vframe;
		rec->conv = NULL;
		snprintf(rec->suffix, 6, "avi");
		snprintf(rec->description, 64, "AVI Video (*.avi)");
		break;
	}

	//
	// create avcodec encoding objects
	//

    av_register_all();
	avcodec_register_all();

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52,60,0)
	rec->enc->fmt = guess_stream_format(f_name, NULL, NULL);
#else
	rec->enc->fmt = av_guess_format(f_name, NULL, NULL);
#endif

	if(rec->enc->fmt == NULL) {
		snprintf(errormessage, 256, "File format %s not supported.\nUnable to start recording %s.", f_name, filename);
		video_rec_free(rec);
		return NULL;
	}

	rec->enc->oc = avformat_alloc_context();
	rec->enc->oc->oformat = rec->enc->fmt;
	snprintf(rec->enc->oc->filename, sizeof(rec->enc->oc->filename), "%s", filename);


	// create the stream for the encoder
	rec->enc->v_st = av_new_stream(rec->enc->oc, 0);
	rec->enc->v_ctx = rec->enc->v_st->codec;

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53,0,0)
	rec->enc->v_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
#else
	rec->enc->v_ctx->codec_type = CODEC_TYPE_VIDEO;
#endif
	rec->enc->v_ctx->codec_id = f_codec_id;

	rec->enc->v_ctx->width = width;
	rec->enc->v_ctx->height = height;
	rec->enc->v_ctx->time_base.den = fps;

//	rec->enc->v_ctx->bit_rate = width*height*4*fps;  // useless ?
	rec->enc->v_ctx->time_base.num = 1;
	rec->enc->v_ctx->pix_fmt = f_pix_fmt;
	rec->enc->v_ctx->coder_type = 1;

	c = avcodec_find_encoder(f_codec_id);
	if (c == NULL) {
		snprintf(errormessage, 256, "Cannot find video codec for %s file.\nUnable to start recording.", f_name);
		video_rec_free(rec);
		return NULL;
	}

    // options for AVCodecContext
    AVDictionary *codec_opts = NULL;

    // parameters given
     snprintf(buf, sizeof(buf), "%d/%d", 1, fps);
     av_dict_set(&codec_opts, "time_base", buf, 0);
     snprintf(buf, sizeof(buf), "%d", width);
     av_dict_set(&codec_opts, "width", buf, 0);
     snprintf(buf, sizeof(buf), "%d", height);
     av_dict_set(&codec_opts, "height", buf, 0);

     av_dict_set(&codec_opts, "codec_type", "AVMEDIA_TYPE_VIDEO", 0);
     av_dict_set(&codec_opts, "pix_fmt", av_get_pix_fmt_name(f_pix_fmt), 0 );

	if(avcodec_open(rec->enc->v_ctx, c) < 0) {
		snprintf(errormessage, 256, "Cannot open video codec %s (at %d fps).\nUnable to start recording. \n%s", c->name, fps, av_get_pix_fmt_name( f_pix_fmt ));
		video_rec_free(rec);
		return NULL;
	}


#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52,80,0)
	if(url_fopen(&rec->enc->oc->pb, filename, URL_WRONLY) < 0) {
#else
	if(avio_open(&rec->enc->oc->pb, filename, AVIO_FLAG_WRITE) < 0) {
#endif
		snprintf(errormessage, 256, "Cannot create temporary file %s.\nUnable to start recording.", filename);
		video_rec_free(rec);
		return NULL;
	}

    // no options for AVFormatContext
	avformat_write_header(rec->enc->oc, NULL);

	// if converter was created, we will need it !
	if (rec->conv != NULL) {
		// create picture bufffer and frame to store converted YUV image
		int size = rec->enc->v_ctx->width * rec->enc->v_ctx->height;
		rec->conv->picture_buf = (uint8_t *) malloc((size * 3) / 2); /* size for YUV 420 */
		rec->conv->picture= avcodec_alloc_frame();
		rec->conv->picture->data[0] = rec->conv->picture_buf;
		rec->conv->picture->data[1] = rec->conv->picture->data[0] + size;
		rec->conv->picture->data[2] = rec->conv->picture->data[1] + size / 4;
		rec->conv->picture->linesize[0] = rec->enc->v_ctx->width;
		rec->conv->picture->linesize[1] = rec->enc->v_ctx->width / 2;
		rec->conv->picture->linesize[2] = rec->enc->v_ctx->width / 2;

		// create conversion context
		rec->conv->img_convert_ctx = sws_getCachedContext(NULL, rec->width, rec->height, PIX_FMT_BGRA,
													rec->enc->v_ctx->width, rec->enc->v_ctx->height, f_pix_fmt,
													SWS_POINT, NULL, NULL, NULL);
		if (rec->conv->img_convert_ctx == NULL){
			snprintf(errormessage, 256, "Could not create conversion context.\nUnable to record to %s.", filename);
			video_rec_free(rec);
			return NULL;
		}

	}
	// encoder buffer : sized to fit the maximum resolution and frame rate.
	rec->enc->vbuf_size = 20000000;
	rec->enc->vbuf_ptr = av_malloc(rec->enc->vbuf_size);

	return rec;
}


/**
 *
 */

int video_rec_stop(video_rec_t *rec)
{
	if (rec == NULL)
		return 0;

	// end file
	av_write_trailer(rec->enc->oc);
	// close file

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52,80,0)
	url_fclose(rec->enc->oc->pb);
#else
	avio_close(rec->enc->oc->pb);
#endif

	int i = 0;
	for(; i < rec->enc->oc->nb_streams; i++) {
		AVStream *st = rec->enc->oc->streams[i];
		avcodec_close(st->codec);
		av_free(st->codec);
		av_free(st);
	}

	return rec->framenum;
}

void video_rec_free(video_rec_t *rec)
{
	if (rec) {
		// free data structures
		if (rec->enc->oc)
			av_free(rec->enc->oc);
		if (rec->enc->vbuf_ptr)
			av_free(rec->enc->vbuf_ptr);
		if (rec->enc)
			free(rec->enc);
		if (rec->conv) {
			free(rec->conv->picture_buf);
			av_free(rec->conv->picture);
			if (rec->conv->img_convert_ctx)
				sws_freeContext(rec->conv->img_convert_ctx);
			free(rec->conv);
		}
		free(rec);
	}
}

/**
 *
 */
void
rec_deliver_vframe(video_rec_t *rec, void *data)
{
	int r;
	AVPacket pkt;

	AVFrame frame;
	memset(&frame, 0, sizeof(frame));
	int linesize = rec->width * 4;
	frame.data[0] = data + linesize * (rec->height - 1);
	frame.linesize[0] = -linesize;
	frame.pts = 1000000LL * rec->framenum / rec->fps;

	r = avcodec_encode_video(rec->enc->v_ctx, rec->enc->vbuf_ptr, rec->enc->vbuf_size, &frame);
	if(r == 0)
		return;

	av_init_packet(&pkt);

	if(rec->enc->v_ctx->coded_frame->pts != AV_NOPTS_VALUE)
		pkt.pts = av_rescale_q(rec->enc->v_ctx->coded_frame->pts,  AV_TIME_BASE_Q, rec->enc->v_st->time_base);

	if(rec->enc->v_ctx->coded_frame->key_frame)
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53,0,0)
		pkt.flags |= AV_PKT_FLAG_KEY;
#else
		pkt.flags |= PKT_FLAG_KEY;
#endif

	pkt.stream_index = rec->enc->v_st->index;
	pkt.data = rec->enc->vbuf_ptr;
	pkt.size = r;

	r = av_interleaved_write_frame(rec->enc->oc, &pkt);
	if(r < 0)
		return;

	// one more frame done !
	rec->framenum++;
}


/**
 *
 */
void
sws_rec_deliver_vframe(video_rec_t *rec, void *data)
{
	int r;
	AVPacket pkt;

	AVFrame frame;
	memset(&frame, 0, sizeof(frame));
	int linesize = rec->width * 4;
	frame.data[0] = data + linesize * (rec->height - 1);
	frame.linesize[0] = -linesize;

	// convert buffer to avcodec frame
	sws_scale(rec->conv->img_convert_ctx, frame.data, frame.linesize, 0, rec->height, rec->conv->picture->data, rec->conv->picture->linesize);
	rec->conv->picture->pts = 1000000LL * rec->framenum / rec->fps;

	r = avcodec_encode_video(rec->enc->v_ctx, rec->enc->vbuf_ptr, rec->enc->vbuf_size, rec->conv->picture);
	if(r == 0)
		return;

	av_init_packet(&pkt);

	if(rec->enc->v_ctx->coded_frame->pts != AV_NOPTS_VALUE)
		pkt.pts = av_rescale_q(rec->enc->v_ctx->coded_frame->pts,  AV_TIME_BASE_Q, rec->enc->v_st->time_base);

	if(rec->enc->v_ctx->coded_frame->key_frame)
#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(53,0,0)
		pkt.flags |= AV_PKT_FLAG_KEY;
#else
		pkt.flags |= PKT_FLAG_KEY;
#endif

	pkt.stream_index = rec->enc->v_st->index;
	pkt.data = rec->enc->vbuf_ptr;
	pkt.size = r;

	r = av_interleaved_write_frame(rec->enc->oc, &pkt);
	if(r < 0)
		return;

	// one more frame done !
	rec->framenum++;

}



