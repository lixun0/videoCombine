#ifndef FFMPEG_DECODE_ENCODE_H
#define FFMPEG_DECODE_ENCODE_H
typedef struct videoFile
{
	AVFormatContext* pFormatCtx;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;
	int videoStream;
	AVStream* video_st;
} videoFile;

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096
#define bool int
#define true 1
#define false 0
#define SHOW(info) printf("%s\n",(info))
#define ERROR(info) fprintf(stderr,"%s\n",(info))
#define STOP printf("press any key to continue..\n");getchar()

int open_video(char* infilename,videoFile* pVideo);

void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename);
int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
                              AVFrame *frame, int *frame_count, AVPacket *pkt, int last);
int write_video_frame(char* outfilename,videoFile* pVideo);
int getVideoFrame(char* filename,videoFile* pVideo,AVFrame* pFrame);
/* select layout with the highest channel count */
int select_channel_layout(AVCodec *codec);
/* just pick the highest supported samplerate */
int select_sample_rate(AVCodec *codec);
/* check that a given sample format is supported by the encoder */
int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt);

void video_decode_example(const char *outfilename, const char *filename);

void video_decode(const char* outfilename,const char* filename);

void video_encode_example(const char *filename, int codec_id);

int video_encode(const char* outfilename,const char* filename);

int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index);
int video_combine(char* filelist[],int fileCnt);
int init_output(const videoFile* inVideo,const char* filename,videoFile* pOutVideo);
//int write_video(const videoFile* inVideo,videoFile* outVideo);
#endif