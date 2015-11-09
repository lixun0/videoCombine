#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <stdlib.h>
#include <libavutil/frame.h>
#include <string.h>
#include <stdio.h>
#include "video_image.h"
//#include "ffmpeg_decode_encode.h"
#include "video_combine.h"
#define bool int
#define true 1
#define false 0

int main(int argc,char* argv[])
{
	char* infilename=argv[1];
	char* outfilename=argv[2];
	av_register_all();
	avcodec_register_all();
	//videoFile video;
	//open_video(infilename,&video);
	//VideoToJPG(&video,outfilename);
	//video_encode(outfilename, infilename);
	//video_combine(argv,argc);
	video_combine2(argv,argc);
}