#ifndef VIDEO_COMBINE_H
#define VIDEO_COMBINE_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <stdlib.h>
#include <libavutil/frame.h>
#include <string.h>
#include <stdio.h>
#include "ffmpeg_decode_encode.h"

AVFrame* frame_combine(AVFrame* frame1,const AVFrame* frame2);
int video_combine(char* filelist[],int fileCnt);
int init_output(const videoFile* inVideo,const char* filename,videoFile* pOutVideo);
//int write_video(const videoFile* inVideo,videoFile* outVideo);
int video_combine2(char* filelist[],int fileCnt);
#endif