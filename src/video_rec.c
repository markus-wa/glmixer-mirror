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

struct ffvhuff_rec {
	AVOutputFormat *fmt;
	AVFormatContext *oc;
	AVCodecContext *v_ctx;
	AVStream *v_st;
	void *vbuf_ptr;
	size_t vbuf_size;
	int64_t pts;
};

struct mpeg_rec {
	AVCodec *codec;
	AVCodecContext *c;
	int out_size, outbuf_size;
	FILE *f;
	AVFrame *picture;
	uint8_t *outbuf, *picture_buf;
	struct SwsContext *img_convert_ctx;
};



video_rec_t *
ffvhuff_rec_init(const char *filename, int width, int height, int fps, char *errormessage)
{
	AVCodec *c;

	video_rec_t *rec = calloc(1, sizeof(video_rec_t));
	rec->ffvhuff = calloc(1, sizeof(struct ffvhuff_rec));

	rec->width = width;
	rec->height = height;
	rec->fps = fps;
	rec->framenum = 0;

	rec->ffvhuff->fmt = av_guess_format(NULL, filename, NULL);
	if(rec->ffvhuff->fmt == NULL) {
		snprintf(errormessage, 256, "Unknown file format. Unable to record to %s.", filename);
		free(rec->ffvhuff);
		free(rec);
		return NULL;
	}

	rec->ffvhuff->oc = avformat_alloc_context();
	rec->ffvhuff->oc->oformat = rec->ffvhuff->fmt;
	snprintf(rec->ffvhuff->oc->filename, sizeof(rec->ffvhuff->oc->filename), "%s", filename);

	rec->ffvhuff->v_st = av_new_stream(rec->ffvhuff->oc, 0);

	rec->ffvhuff->v_ctx = rec->ffvhuff->v_st->codec;
	rec->ffvhuff->v_ctx->codec_type = CODEC_TYPE_VIDEO;
	rec->ffvhuff->v_ctx->codec_id = CODEC_ID_FFVHUFF;

	rec->ffvhuff->v_ctx->width = width;
	rec->ffvhuff->v_ctx->height = height;
	rec->ffvhuff->v_ctx->time_base.den = fps;
	rec->ffvhuff->v_ctx->time_base.num = 1;
	rec->ffvhuff->v_ctx->pix_fmt = PIX_FMT_BGRA;
	rec->ffvhuff->v_ctx->coder_type = 1;

	if(av_set_parameters(rec->ffvhuff->oc, NULL) < 0) {
		snprintf(errormessage, 256, "Invalid output format parameters. Unable to record to %s.", filename);
		free(rec->ffvhuff->oc);
		free(rec->ffvhuff);
		free(rec);
		return NULL;
	}

	dump_format(rec->ffvhuff->oc, 0, filename, 1);

	c = avcodec_find_encoder(rec->ffvhuff->v_ctx->codec_id);
	if (!c) {
		snprintf(errormessage, 256, "Could not find video codec. Unable to record to %s.", filename);
		free(rec->ffvhuff->oc);
		free(rec->ffvhuff);
		free(rec);
		return NULL;
	}

	if(avcodec_open(rec->ffvhuff->v_ctx, c) < 0) {
		snprintf(errormessage, 256, "Could not open video codec. Unable to record to %s.", filename);
		free(rec->ffvhuff->oc);
		free(rec->ffvhuff);
		free(rec);
		return NULL;
	}

	if(url_fopen(&rec->ffvhuff->oc->pb, filename, URL_WRONLY) < 0) {
		snprintf(errormessage, 256, "Could not open temporary file %s for writing.", filename);
		free(rec->ffvhuff->oc);
		free(rec->ffvhuff);
		free(rec);
		return NULL;
	}

	/* write the stream header, if any */
	av_write_header(rec->ffvhuff->oc);

	rec->ffvhuff->vbuf_size = 2000000;
	rec->ffvhuff->vbuf_ptr = av_malloc(rec->ffvhuff->vbuf_size);

	return rec;
}


/**
 *
 */

void
ffvhuff_rec_stop(video_rec_t *rec)
{
	if (rec == NULL)
		return;

	int i;

	av_write_trailer(rec->ffvhuff->oc);

	for(i = 0; i < rec->ffvhuff->oc->nb_streams; i++) {
		AVStream *st = rec->ffvhuff->oc->streams[i];
		avcodec_close(st->codec);
		free(st->codec);
		free(st);
	}

	url_fclose(rec->ffvhuff->oc->pb);
	free(rec->ffvhuff->oc);
	free(rec->ffvhuff->vbuf_ptr);
	free(rec->ffvhuff);
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

	r = avcodec_encode_video(rec->ffvhuff->v_ctx, rec->ffvhuff->vbuf_ptr, rec->ffvhuff->vbuf_size, &frame);
	if(r == 0)
		return;

	av_init_packet(&pkt);

	if(rec->ffvhuff->v_ctx->coded_frame->pts != AV_NOPTS_VALUE)
		pkt.pts = av_rescale_q(rec->ffvhuff->v_ctx->coded_frame->pts,  AV_TIME_BASE_Q, rec->ffvhuff->v_st->time_base);

	if(rec->ffvhuff->v_ctx->coded_frame->key_frame)
		pkt.flags |= PKT_FLAG_KEY;

	pkt.stream_index = rec->ffvhuff->v_st->index;
	pkt.data = rec->ffvhuff->vbuf_ptr;
	pkt.size = r;

	av_interleaved_write_frame(rec->ffvhuff->oc, &pkt);

	// one more frame done !
	rec->framenum++;
}


video_rec_t *
mpeg_rec_init(const char *filename, int width, int height, int fps, char *errormessage)
{
	video_rec_t *rec = calloc(1, sizeof(video_rec_t));
	rec->mpeg = calloc(1, sizeof(struct mpeg_rec));

	rec->width = width;
	rec->height = height;
	rec->fps = fps;
	rec->framenum = 0;

	/* find the mpeg1 video encoder */
	rec->mpeg->codec = avcodec_find_encoder(CODEC_ID_MPEG1VIDEO);
	if (!rec->mpeg->codec) {
		snprintf(errormessage, 256, "Could not find video codec. Unable to record to %s.", filename);
		free(rec->mpeg);
		free(rec);
		return NULL;
	}

	rec->mpeg->c= avcodec_alloc_context();
	rec->mpeg->picture= avcodec_alloc_frame();

	/* put sample parameters */
	rec->mpeg->c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	rec->mpeg->c->width = width;
	rec->mpeg->c->height = height;
	/* frames per second */
	rec->mpeg->c->time_base= (AVRational){1,fps};
	/* emit one intra frame every ten frames */
	rec->mpeg->c->gop_size = 10;
	rec->mpeg->c->max_b_frames=1;
	rec->mpeg->c->pix_fmt = PIX_FMT_YUV420P;

	/* open it */
	if (avcodec_open(rec->mpeg->c, rec->mpeg->codec) < 0) {
		snprintf(errormessage, 256, "Could not open video codec. Unable to record to %s.", filename);
		av_free(rec->mpeg->c);
		av_free(rec->mpeg->picture);
		free(rec->mpeg);
		free(rec);
		return NULL;
	}

	/* alloc image and output buffer */
	rec->mpeg->outbuf_size = 1000000;
	rec->mpeg->outbuf = (uint8_t *) malloc(rec->mpeg->outbuf_size);
	int size = rec->mpeg->c->width * rec->mpeg->c->height;
	rec->mpeg->picture_buf = (uint8_t *) malloc((size * 3) / 2); /* size for YUV 420 */

	rec->mpeg->picture->data[0] = rec->mpeg->picture_buf;
	rec->mpeg->picture->data[1] = rec->mpeg->picture->data[0] + size;
	rec->mpeg->picture->data[2] = rec->mpeg->picture->data[1] + size / 4;
	rec->mpeg->picture->linesize[0] = rec->mpeg->c->width;
	rec->mpeg->picture->linesize[1] = rec->mpeg->c->width / 2;
	rec->mpeg->picture->linesize[2] = rec->mpeg->c->width / 2;

	// create conversion context
	rec->mpeg->img_convert_ctx = sws_getContext(rec->width, rec->height, PIX_FMT_RGB24,
			rec->mpeg->c->width, rec->mpeg->c->height, PIX_FMT_YUV420P,
			SWS_POINT, NULL, NULL, NULL);
	if (rec->mpeg->img_convert_ctx == NULL){
		snprintf(errormessage, 256, "Could not create conversion context. Unable to record to %s.", filename);
		av_free(rec->mpeg->c);
		av_free(rec->mpeg->picture);
		free(rec->mpeg);
		free(rec);
		return NULL;
	}

	rec->mpeg->f = fopen( filename, "wb");
	if (!rec->mpeg->f) {
		snprintf(errormessage, 256, "Could not open temporary file %s for writing.", filename);
		sws_freeContext(rec->mpeg->img_convert_ctx);
		av_free(rec->mpeg->c);
		av_free(rec->mpeg->picture);
		free(rec->mpeg);
		free(rec);
		return NULL;
	}

	return rec;
}



void  mpeg_rec_stop(video_rec_t *rec)
{
	if (rec == NULL)
		return;

	/* get the delayed frames */
	for(; rec->mpeg->out_size;) {
		rec->mpeg->out_size = avcodec_encode_video(rec->mpeg->c, rec->mpeg->outbuf, rec->mpeg->outbuf_size, NULL);
		fwrite(rec->mpeg->outbuf, 1, rec->mpeg->out_size, rec->mpeg->f);
	}

	/* add sequence end code to have a real mpeg file */
	rec->mpeg->outbuf[0] = 0x00;
	rec->mpeg->outbuf[1] = 0x00;
	rec->mpeg->outbuf[2] = 0x01;
	rec->mpeg->outbuf[3] = 0xb7;
	fwrite(rec->mpeg->outbuf, 1, 4, rec->mpeg->f);
	fclose(rec->mpeg->f);
	free(rec->mpeg->picture_buf);
	free(rec->mpeg->outbuf);

	// free avcodec stuff
	avcodec_close(rec->mpeg->c);
	av_free(rec->mpeg->c);
	av_free(rec->mpeg->picture);

	// free context converter
	sws_freeContext(rec->mpeg->img_convert_ctx);

	free(rec->mpeg);
	free(rec);
}


/**
 *
 */
void
mpeg_rec_deliver_vframe(video_rec_t *rec, void *data)
{
//	uint8_t *frame [4];
	const uint8_t *frame [4];
	int 	linesize [4];

	// setup the frame pointers to tmpframe for reading the frame backward (y inverted in opengl)
	linesize[0] = - rec->width * 3;
	frame[0] = (uint8_t *) data + (rec->width) * (rec->height - 1) * 3;

	// convert buffer to avcodec frame
	sws_scale(rec->mpeg->img_convert_ctx, frame, linesize, 0, rec->height, rec->mpeg->picture->data, rec->mpeg->picture->linesize);

	// set timing for frame and increment frame counter
	rec->mpeg->picture->pts = 1000000LL * rec->framenum / rec->fps;
	rec->framenum++;

	/* encode the image */
	rec->mpeg->out_size = avcodec_encode_video(rec->mpeg->c, rec->mpeg->outbuf, rec->mpeg->outbuf_size, rec->mpeg->picture);
	fwrite(rec->mpeg->outbuf, 1, rec->mpeg->out_size, rec->mpeg->f);

}
