/*
 * video_rec.c
 *
 *  Created on: Mar 16, 2011
 *      Author: bh
 */

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

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
	AVCodec *c;

	video_rec_t *rec = calloc(1, sizeof(video_rec_t));
	rec->enc = calloc(1, sizeof(struct encoder));

	// fill in basic infor
	rec->width = width;
	rec->height = height;
	rec->fps = fps;
	rec->framenum = 0;

	// setup according to format
	char f_name[32];
	enum CodecID f_codec_id = CODEC_ID_NONE;
	enum PixelFormat f_pix_fmt =  PIX_FMT_NONE;
	switch (f){
	case FORMAT_MPG_MPEG1:
		snprintf(f_name, 32, "mpeg");
		f_codec_id = CODEC_ID_MPEG1VIDEO;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &mpeg_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		break;
	case FORMAT_WMV_WMV1:
		snprintf(f_name, 32, "avi");
		f_codec_id = CODEC_ID_WMV1;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &mpeg_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		break;
	case FORMAT_MP4_MPEG4:
		snprintf(f_name, 32, "mp4");
		f_codec_id = CODEC_ID_MPEG4;
		f_pix_fmt =  PIX_FMT_YUV420P;
		rec->pt2RecordingFunction = &mpeg_rec_deliver_vframe;
		rec->conv = calloc(1, sizeof(struct converter));
		break;
	default:
	case FORMAT_AVI_FFVHUFF:
		snprintf(f_name, 32, "avi");
		f_codec_id = CODEC_ID_FFVHUFF;
		f_pix_fmt =  PIX_FMT_BGRA;
		rec->pt2RecordingFunction = &ffvhuff_rec_deliver_vframe;
		rec->conv = NULL;
	}

	//
	// create avcodec encoding objects
	//

	rec->enc->fmt = av_guess_format(f_name, NULL, NULL);
	if(rec->enc->fmt == NULL) {
		snprintf(errormessage, 256, "%s file format not supported. Unable to record to %s.", f_name, filename);
		free(rec->enc);
		free(rec);
		return NULL;
	}

	rec->enc->oc = avformat_alloc_context();
	rec->enc->oc->oformat = rec->enc->fmt;
	snprintf(rec->enc->oc->filename, sizeof(rec->enc->oc->filename), "%s", filename);

	rec->enc->v_st = av_new_stream(rec->enc->oc, 0);

	rec->enc->v_ctx = rec->enc->v_st->codec;
	rec->enc->v_ctx->codec_type = CODEC_TYPE_VIDEO;
	rec->enc->v_ctx->codec_id = f_codec_id;

	rec->enc->v_ctx->width = width;
	rec->enc->v_ctx->height = height;
	rec->enc->v_ctx->time_base.den = fps;
	rec->enc->v_ctx->time_base.num = 1;
	rec->enc->v_ctx->pix_fmt = f_pix_fmt;
	rec->enc->v_ctx->coder_type = 1;

	if(av_set_parameters(rec->enc->oc, NULL) < 0) {
		snprintf(errormessage, 256, "Invalid output format parameters.\nUnable to start recording.");
		free(rec->enc->oc);
		free(rec->enc);
		free(rec);
		return NULL;
	}

	dump_format(rec->enc->oc, 0, filename, 1);

	avcodec_register_all();

	c = avcodec_find_encoder(rec->enc->v_ctx->codec_id);
	if (!c) {
		snprintf(errormessage, 256, "Cannot find video codec %s.\nUnable to start recording.", c->name);
		free(rec->enc->oc);
		free(rec->enc);
		free(rec);
		return NULL;
	}

	if(avcodec_open(rec->enc->v_ctx, c) < 0) {
		snprintf(errormessage, 256, "Cannot open video codec %s at %d fps.\nUnable to start recording.", c->name, fps);
		free(rec->enc->oc);
		free(rec->enc);
		free(rec);
		return NULL;
	}

	if(url_fopen(&rec->enc->oc->pb, filename, URL_WRONLY) < 0) {
		snprintf(errormessage, 256, "Could not open temporary file %s for writing.", filename);
		free(rec->enc->oc);
		free(rec->enc);
		free(rec);
		return NULL;
	}

	/* write the stream header, if any */
	av_write_header(rec->enc->oc);

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
		rec->conv->img_convert_ctx = sws_getContext(rec->width, rec->height, PIX_FMT_BGRA,
													rec->enc->v_ctx->width, rec->enc->v_ctx->height, f_pix_fmt,
													SWS_POINT, NULL, NULL, NULL);
		if (rec->conv->img_convert_ctx == NULL){
			snprintf(errormessage, 256, "Could not create conversion context. Unable to record to %s.", filename);
			free(rec->conv->picture_buf);
			av_free(rec->conv->picture);
			free(rec->enc->oc);
			free(rec->enc);
			free(rec->conv);
			free(rec);
			return NULL;
		}
	}

	// encoder buffer
	rec->enc->vbuf_size = 2000000;
	rec->enc->vbuf_ptr = av_malloc(rec->enc->vbuf_size);

	return rec;
}


/**
 *
 */

void video_rec_stop(video_rec_t *rec)
{
	if (rec == NULL)
		return;

	// end file
	av_write_trailer(rec->enc->oc);
	int i = 0;
	for(; i < rec->enc->oc->nb_streams; i++) {
		AVStream *st = rec->enc->oc->streams[i];
		avcodec_close(st->codec);
		free(st->codec);
		free(st);
	}
	// close file
	url_fclose(rec->enc->oc->pb);

	// free data structures
	free(rec->enc->oc);
	free(rec->enc->vbuf_ptr);
	free(rec->enc);
	if (rec->conv) {
		free(rec->conv->picture_buf);
		av_free(rec->conv->picture);
		sws_freeContext(rec->conv->img_convert_ctx);
		free(rec->conv);
	}
	free(rec);
}


/**
 *
 */
void
ffvhuff_rec_deliver_vframe(video_rec_t *rec, void *data)
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
		pkt.flags |= PKT_FLAG_KEY;

	pkt.stream_index = rec->enc->v_st->index;
	pkt.data = rec->enc->vbuf_ptr;
	pkt.size = r;

	av_interleaved_write_frame(rec->enc->oc, &pkt);

	// one more frame done !
	rec->framenum++;
}


/**
 *
 */
void
mpeg_rec_deliver_vframe(video_rec_t *rec, void *data)
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
		pkt.flags |= PKT_FLAG_KEY;

	pkt.stream_index = rec->enc->v_st->index;
	pkt.data = rec->enc->vbuf_ptr;
	pkt.size = r;

	av_interleaved_write_frame(rec->enc->oc, &pkt);

	// one more frame done !
	rec->framenum++;
}


