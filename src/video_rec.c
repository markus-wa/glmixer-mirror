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
	AVCodec *c = NULL;

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
//		snprintf(f_name, 9, "rawvideo");
		f_codec_id = CODEC_ID_RAWVIDEO;
		f_pix_fmt =  PIX_FMT_BGRA;
		rec->pt2RecordingFunction = &rec_deliver_vframe;
		rec->conv = NULL;
		snprintf(rec->suffix, 6, "avi");
		snprintf(rec->description, 64, "AVI Video (*.avi)");
	}

	//
	// create avcodec encoding objects
	//

    av_register_all();
	rec->enc->fmt = av_guess_format(f_name, NULL, NULL);
	if(rec->enc->fmt == NULL) {
		snprintf(errormessage, 256, "File format %s not supported.\nUnable to start recording.", f_name, filename);
		video_rec_free(rec);
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
		video_rec_free(rec);
		return NULL;
	}

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52,80,0)
	dump_format(rec->enc->oc, 0, filename, 1);
#else
	av_dump_format(rec->enc->oc, 0, filename, 1);
#endif

	avcodec_register_all();

	c = avcodec_find_encoder(rec->enc->v_ctx->codec_id);
	if (c == NULL) {
		snprintf(errormessage, 256, "Cannot find video codec for %s file.\nUnable to start recording.", f_name);
		video_rec_free(rec);
		return NULL;
	}

	if(avcodec_open(rec->enc->v_ctx, c) < 0) {
		snprintf(errormessage, 256, "Cannot open video codec %s (at %d fps).\nUnable to start recording.", c->name, rec->enc->v_ctx->time_base.den);
		video_rec_free(rec);
		return NULL;
	}

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(52,80,0)
	if(url_fopen(&rec->enc->oc->pb, filename, URL_WRONLY) < 0) {
#else
	if(avio_open(&rec->enc->oc->pb, filename, URL_WRONLY) < 0) {
#endif
		snprintf(errormessage, 256, "Cannot create temporary file %s.\nUnable to start recording.", filename);
		video_rec_free(rec);
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
		rec->conv->img_convert_ctx = sws_getCachedContext(NULL, rec->width, rec->height, PIX_FMT_BGRA,
													rec->enc->v_ctx->width, rec->enc->v_ctx->height, f_pix_fmt,
													SWS_POINT, NULL, NULL, NULL);
		if (rec->conv->img_convert_ctx == NULL){
			snprintf(errormessage, 256, "Could not create conversion context.\nUnable to record to %s.", filename);
			video_rec_free(rec);
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
		avcodec_default_free_buffers (st->codec);
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
		pkt.flags |= PKT_FLAG_KEY;

	pkt.stream_index = rec->enc->v_st->index;
	pkt.data = rec->enc->vbuf_ptr;
	pkt.size = r;

	av_interleaved_write_frame(rec->enc->oc, &pkt);

	// one more frame done !
	rec->framenum++;

}



