#ifndef FFMEG_GRAB_H
#define FFMPEG_GRAB_H

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,char *filename);
static int decode_write_frame(const char *outfilename, AVCodecContext *avctx,AVFrame *frame, int *frame_count, AVPacket *pkt, int last);

#endif