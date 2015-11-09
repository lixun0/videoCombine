#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <stdlib.h>
#include <libavutil/frame.h>
#include <string.h>
#include <stdio.h>
#include "ffmpeg_decode_encode.h"


int open_video(char* infilename,videoFile* pVideo)
{
	pVideo->pFormatCtx=avformat_alloc_context();
	if(avformat_open_input(&(pVideo->pFormatCtx), infilename, NULL, NULL)!=0){fprintf(stderr,"cant open %s!\n",infilename);return -1;}
	if(avformat_find_stream_info(pVideo->pFormatCtx,NULL)<0){fprintf(stderr,"cant find streaminfo!\n");return -1;}
	int videoStream,i;
	for(i=0;i<pVideo->pFormatCtx->nb_streams; i++)
		if(pVideo->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){videoStream=i;break;}
	if(videoStream==-1)return -1;
	pVideo->pCodecCtx=pVideo->pFormatCtx->streams[videoStream]->codec;
	pVideo->pCodec=avcodec_find_decoder(pVideo->pCodecCtx->codec_id); 
	if(pVideo->pCodec==NULL){fprintf(stderr, "Unsupported codec!\n");return -1;}
	if(avcodec_open2(pVideo->pCodecCtx,pVideo->pCodec,NULL)!=0){fprintf(stderr,"codec open fail!\n");return -1;}
	pVideo->videoStream=videoStream;
	return 0;
}

int close_video(char* outfilename,videoFile* pVideo)
{
}

void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize,
                     char *filename)
{
    FILE *f;
    int i;

    f=fopen(filename,"w");
    fprintf(f,"P5\n%d %d\n%d\n",xsize,ysize,255);
    for(i=0;i<ysize;i++)
        fwrite(buf + i * wrap,1,xsize,f);
    fclose(f);
}

int decode_write_frame(const char *outfilename, AVCodecContext *avctx,
                              AVFrame *frame, int *frame_count, AVPacket *pkt, int last)
{
    int len, got_frame;
    char buf[1024];

    len = avcodec_decode_video2(avctx, frame, &got_frame, pkt);
    if (len < 0) {
        fprintf(stderr, "Error while decoding frame %d\n len=%d\n", *frame_count,len);
        return len;
    }
    if (got_frame) {
        printf("Saving %sframe %3d\n", last ? "last " : "", *frame_count);
        fflush(stdout);

        /* the picture is allocated by the decoder, no need to free it */
        snprintf(buf, sizeof(buf), outfilename, *frame_count);
        pgm_save(frame->data[0], frame->linesize[0],
                 avctx->width, avctx->height, buf);
        (*frame_count)++;
    }
    if (pkt->data) {
        pkt->size -= len;
        pkt->data += len;
    }
    return 0;
}

/*int write_video_frame(char* outfilename,videoFile* pVideo)
{
	int videoStream=pVideo->videoStream;
	AVFrame *pFrame=NULL;
	pFrame=av_frame_alloc();
	if(pFrame==NULL){fprintf(stderr,"alloc frame fail!\n");return -1;}
	AVPacket packet;
	int frameCnt=0;
	while(av_read_frame(pVideo->pFormatCtx,&packet)>=0)
	{
		if(packet.stream_index==videoStream)
		{
			char numstr[25];
			sprintf(numstr,"%d",frameCnt);
			char* file=strcat(numstr,outfilename);
			printf("%d\n",packet.size);
			while(packet.size>0){decode_write_frame(file,pVideo->pCodecCtx,pFrame,&frameCnt,&packet,0);printf("%d\n",packet.size);}
		}
	}
	return 0;
}*/

int getVideoFrame(char* filename,videoFile* pVideo,AVFrame* pFrame)
{
	int videoStream=pVideo->videoStream;
	if(pFrame==NULL){fprintf(stderr,"alloc frame fail!\n");return -1;}
	AVPacket packet;
	int frameCnt=0;
	while(av_read_frame(pVideo->pFormatCtx,&packet)>=0)
	{
		if(packet.stream_index==videoStream)
		{
			int got_frame;
			int len = avcodec_decode_video2(pVideo->pCodecCtx,pFrame,&got_frame, &packet);
			if (len < 0) {
				fprintf(stderr, "Error while decoding frame!\n");
				return -1;
				}
			if (packet.data) {
				packet.size -= len;
				packet.data += len;
				}
			if(!got_frame){
				fprintf(stderr,"cant got frame!\n");
			}
		}
		printf("%d\n",++frameCnt);
	}
	return 0;
}

int select_channel_layout(AVCodec *codec)
{
    const uint64_t *p;
    uint64_t best_ch_layout = 0;
    int best_nb_channels   = 0;

    if (!codec->channel_layouts)
        return AV_CH_LAYOUT_STEREO;

    p = codec->channel_layouts;
    while (*p) {
        int nb_channels = av_get_channel_layout_nb_channels(*p);

        if (nb_channels > best_nb_channels) {
            best_ch_layout    = *p;
            best_nb_channels = nb_channels;
        }
        p++;
    }
    return best_ch_layout;
}

int select_sample_rate(AVCodec *codec)
{
    const int *p;
    int best_samplerate = 0;

    if (!codec->supported_samplerates)
        return 44100;

    p = codec->supported_samplerates;
    while (*p) {
        best_samplerate = FFMAX(*p, best_samplerate);
        p++;
    }
    return best_samplerate;
}

/* check that a given sample format is supported by the encoder */
int check_sample_fmt(AVCodec *codec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *p = codec->sample_fmts;

    while (*p != AV_SAMPLE_FMT_NONE) {
        if (*p == sample_fmt)
            return 1;
        p++;
    }
    return 0;
}

void video_decode(const char* outfilename,const char* filename)
{
	videoFile video,*pVideo;
	open_video(filename,&video);
	pVideo=&video;
	AVCodecContext* c=pVideo->pCodecCtx;
	int frame_count;
	FILE *f;
    AVFrame *frame;
    uint8_t inbuf[INBUF_SIZE + FF_INPUT_BUFFER_PADDING_SIZE];
    AVPacket avpkt;
	
	av_init_packet(&avpkt);
	/* set end of buffer to 0 (this ensures that no overreading happens for damaged mpeg streams) */
    memset(inbuf + INBUF_SIZE, 0, FF_INPUT_BUFFER_PADDING_SIZE);

    printf("Decode video file %s to %s\n", filename, outfilename);
	f = fopen(filename, "rb");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", filename);
        exit(1);
    }

    frame = avcodec_alloc_frame();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }

    frame_count = 0;
    for(;;) {
        avpkt.size = fread(inbuf, 1, INBUF_SIZE, f);
        if (avpkt.size == 0)
            break;

        /* NOTE1: some codecs are stream based (mpegvideo, mpegaudio)
           and this is the only method to use them because you cannot
           know the compressed data size before analysing it.

           BUT some other codecs (msmpeg4, mpeg4) are inherently frame
           based, so you must call them with all the data for one
           frame exactly. You must also initialize 'width' and
           'height' before initializing them. */

        /* NOTE2: some codecs allow the raw parameters (frame size,
           sample rate) to be changed at any frame. We handle this, so
           you should also take care of it */

        /* here, we use a stream based decoder (mpeg1video), so we
           feed decoder and see if it could decode a frame */
        avpkt.data = inbuf;
        while (avpkt.size > 0)
            if (decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 0) < 0)
                exit(1);
    }

    /* some codecs, such as MPEG, transmit the I and P frame with a
       latency of one frame. You must do the following to have a
       chance to get the last frame of the video */
    avpkt.data = NULL;
    avpkt.size = 0;
    decode_write_frame(outfilename, c, frame, &frame_count, &avpkt, 1);

    fclose(f);

    avcodec_close(c);
    av_free(c);
    avcodec_free_frame(&frame);
    printf("\n");
}

int video_encode(const char* outfilename,const char* filename)
{
	
	videoFile video;
	if(open_video(filename,&video)!=0){fprintf(stderr,"open video fail\n");return -1;};
	//av_dump_format(video.pFormatCtx, 0, filename, 0);
	AVFormatContext* pFormatCtx; 
    AVOutputFormat* fmt;  
    AVStream* video_st;  
    AVCodecContext* pCodecCtx;  
    AVCodec* pCodec;  
  
    //uint8_t* picture_buf;  
     
    int size;  
	  
    int in_w=video.pCodecCtx->width,in_h=video.pCodecCtx->height;//宽高 
    const char* out_file = outfilename;                    //输出文件路径  
  
    av_register_all();  
    //方法1.组合使用几个函数  
    //pFormatCtx = avformat_alloc_context();  
    //猜格式  
    //fmt = av_guess_format(NULL, out_file, NULL);  
    //pFormatCtx->oformat = fmt;  
      
    //方法2.更加自动化一些  
    avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);  
    fmt = pFormatCtx->oformat;  
  
  
    //注意输出路径  
    if (avio_open(&pFormatCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0)  
    {  
        printf("输出文件打开失败");  
        return -1;  
    }  
    video_st = av_new_stream(pFormatCtx, 0);  
    if (video_st==NULL)  
    {  
        return -1;  
    }
	//printf("%d\n",video_st->codec);
	//exit(1);
    pCodecCtx = video_st->codec;  
    pCodecCtx->codec_id = fmt->video_codec;  
    //pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;  
	pCodecCtx->codec_type=video.pCodecCtx->codec_type;
    pCodecCtx->pix_fmt = PIX_FMT_YUV420P;
	//pCodecCtx->pix_fmt=video.pCodecCtx->pix_fmt;
	//pCodecCtx->pix_fmt=PIX_FMT_RGB24;
    pCodecCtx->width = in_w;
    pCodecCtx->height = in_h;  
    pCodecCtx->time_base.num = 1;  
    pCodecCtx->time_base.den =25; 
	//pCodecCtx->time_base.num=video.pCodecCtx->time_base.num;
	//pCodecCtx->time_base.den=video.pCodecCtx->time_base.den;
    pCodecCtx->bit_rate = video.pCodecCtx->bit_rate;    
    pCodecCtx->gop_size=video.pCodecCtx->gop_size; 
    //H264  
    //pCodecCtx->me_range = 16;  
    //pCodecCtx->max_qdiff = 4;  
    pCodecCtx->qmin = video.pCodecCtx->qmin;  
    pCodecCtx->qmax = video.pCodecCtx->qmax;  
    pCodecCtx->qcompress = video.pCodecCtx->qcompress;  
    //输出格式信息  
    av_dump_format(pFormatCtx, 0, out_file, 1);  
  
    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);  
    if (!pCodec)  
    {  
        fprintf(stderr,"没有找到合适的编码器！\n");  
        return -1;  
    }  
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)  
    {  
        fprintf(stderr,"编码器打开失败！\n");  
        return -1;  
    }  
    AVFrame* picture = avcodec_alloc_frame();
	if (picture==NULL)
	{
		printf("avcodec alloc frame failed!\n");
		exit (1);
	}
    //size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);  
    //picture_buf = (uint8_t *)av_malloc(size*sizeof(uint8_t));  
    //avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);  
  
    //写文件头  
    if(avformat_write_header(pFormatCtx,NULL)<0){fprintf(stderr,"写入文件头失败\n");return -1;};  
  
    AVPacket pkt;  
	int frameFinished=0;
	int i=0;
	int y_size = pCodecCtx->width * pCodecCtx->height;
	while(av_read_frame(video.pFormatCtx,&pkt)==0)
	{
		printf("loop start\n");
		if(pkt.stream_index==video.videoStream)
		{
			int r=avcodec_decode_video2(video.pCodecCtx, picture, &frameFinished, &pkt);
			printf("%d %d\n",frameFinished,r);
			if(frameFinished&&r>=0)
			   {
					int got_picture=0;
					AVPacket inPKT;
					
					av_new_packet(&inPKT,y_size*3);
					if(avcodec_encode_video2(pCodecCtx, &inPKT,picture, &got_picture)>=0)
					{
						if(got_picture!=1){fprintf(stderr,"no picture got\n");return -1;}
						printf("编码成功第%d帧！\n",++i);  
						inPKT.stream_index = video_st->index;  
						av_write_frame(pFormatCtx, &inPKT);  
						av_free_packet(&inPKT);  
					}
					else{fprintf(stderr,"encode fail\n");return -1;}
			   }
			else{fprintf(stderr,"cant get frame %d\n",++i);}
		}
		av_free_packet(&pkt);
		printf("loop finished\n");
	}
	printf("开始写文件尾\n");
	av_write_trailer(pFormatCtx);
	if (video_st)  
    {  
        avcodec_close(video_st->codec);  
        av_free(picture);  
        //av_free(picture_buf);  
    }  
    avio_close(pFormatCtx->pb);  
    avformat_free_context(pFormatCtx);  
}

/*int write_video(const videoFile* inVideo,videoFile* outVideo)
{
	AVCodecContext* pCodecCtx=outVideo->pCodecCtx;
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);  
	AVStream* video_st = outVideo->video_st;  
    if (video_st==NULL)  
    {  
        return -1;  
    }  
    if (!pCodec)  
    {  
        fprintf(stderr,"没有找到合适的编码器！\n");  
        return -1;  
    }  
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0)  
    {  
        fprintf(stderr,"编码器打开失败！\n");  
        return -1;  
    }  
    AVFrame* picture = avcodec_alloc_frame();
	if (picture==NULL)
	{
		printf("avcodec alloc frame failed!\n");
		exit (1);
	}
	
	AVPacket pkt;  
	int frameFinished=0,i=0;
	uint64_t tmpPTS=0;
	static uint64_t PTS=0;
	while(av_read_frame(inVideo->pFormatCtx,&pkt)>=0)
	{
		//printf("loop start\n");
		if(pkt.stream_index==inVideo->videoStream)
		{
			i++;
			printf("pkt->pts:%d\n",pkt.pts);
			int r=avcodec_decode_video2(inVideo->pCodecCtx, picture, &frameFinished, &pkt);
			//printf("%d %d %d\n",picture->linesize[0],picture->linesize[1],picture->linesize[2]);
			if(frameFinished&&r>=0)
			{
					int got_picture=0;
					AVPacket inPKT;
					int y_size = pCodecCtx->width * pCodecCtx->height;
					av_new_packet(&inPKT,y_size);
					printf("picture->pts:%d\n",picture->pts);
					picture->pts=inPKT.pts;
					if(avcodec_encode_video2(pCodecCtx, &inPKT,picture, &got_picture)>=0)
					{
						printf("inPKT.pts=%d\n",inPKT.pts);
						printf("inPKT.dts=%d\n",inPKT.dts);
						(picture->pts)++;
						inPKT.pts+=PTS;
						//inPKT.dts+=PTS;
						tmpPTS=inPKT.pts;
						if(got_picture!=1){fprintf(stderr,"no picture got\n");continue;}
						
					if (got_picture != 0) {
						if (inPKT.pts != AV_NOPTS_VALUE) {
							inPKT.pts = av_rescale_q(inPKT.pts, inVideo->pCodecCtx->time_base,video_st->time_base);
							}
						if (inPKT.dts != AV_NOPTS_VALUE) {
							inPKT.dts = av_rescale_q(inPKT.dts, inVideo->pCodecCtx->time_base,video_st->time_base);
							}
							inPKT.stream_index = video_st->index;
						} else {
							//return -1;
						}

						av_write_frame(outVideo->pFormatCtx, &inPKT);  
						printf("encoded %drd frame\n",i); 
						av_free_packet(&inPKT);  
					}
					else{fprintf(stderr,"encode fail\n");return -1;}
			}
			else{fprintf(stderr,"cant get frame %d\n",i);}
		}
		av_free_packet(&pkt);
	}
	PTS=tmpPTS;
	if (video_st)  
    {  
        avcodec_close(video_st->codec);  
        av_free(picture);  
    }  
	printf("写入完成\n");
	return 0;
}

void set_opt(AVDictionary** codec_options)
{
	av_dict_set(codec_options,"preset","ultrafast",0);
	av_dict_set(codec_options,"qpmin","0",0);
	av_dict_set(codec_options,"qpmax","69",0);
	av_dict_set(codec_options,"vcodec","libx264",0);
	av_dict_set(codec_options,"tune","zerolatency",0);
	//av_dict_set(codec_options,"threads","12",0);
	//av_dict_set(codec_options,"bframes","0",0);
	//av_dict_set(codec_options,"me_range","16",0);
	//av_dict_set(codec_options,"qcomp","0",0);
	//av_dict_set(codec_options,"profile","baseline",0);
	//av_dict_set(codec_options,"fast_pskip","1",0);
	//av_dict_set(codec_options,"open_gop","0",0);
	//av_dict_set(codec_options,"qpstep","4",0);
	//av_dict_set(codec_options,"ip_ratio","1.4",0);
	//av_dict_set(codec_options,"aq","0",0);
}

int init_output(const videoFile* inVideo,const char* filename,videoFile* pOutVideo)
{
    pOutVideo->video_st = av_new_stream(pOutVideo->pFormatCtx, 0);
	if (pOutVideo->video_st==NULL)  
    {  
        return -1;  
    }
	
	AVOutputFormat* fmt = pOutVideo->pFormatCtx->oformat;
	pOutVideo->pCodecCtx = pOutVideo->video_st->codec;  
    pOutVideo->pCodecCtx->codec_id = fmt->video_codec;
	
	//if(pOutVideo->pCodecCtx->codec_id==AV_CODEC_ID_H264)SHOW("H264!\n");
    pOutVideo->pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;  
	pOutVideo->pCodecCtx->pix_fmt=PIX_FMT_YUV420P;
    pOutVideo->pCodecCtx->width = inVideo->pCodecCtx->width;
    pOutVideo->pCodecCtx->height = inVideo->pCodecCtx->height;
	
	pOutVideo->pCodecCtx->time_base.num = 1;    
    pOutVideo->pCodecCtx->time_base.den =25; 
	
    //pOutVideo->pCodecCtx->bit_rate = inVideo->pCodecCtx->bit_rate;    
    //pOutVideo->pCodecCtx->gop_size=inVideo->pCodecCtx->gop_size; 
    //pOutVideo->pCodecCtx->qmin = inVideo->pCodecCtx->qmin;  
    //pOutVideo->pCodecCtx->qmax = inVideo->pCodecCtx->qmax;
	pOutVideo->pCodecCtx->qcompress = 0;  
	//pOutVideo->pCodecCtx->max_b_frames=0;
	//H264  
	pOutVideo->pCodecCtx->me_range=1;
	pOutVideo->pCodecCtx->max_qdiff = 4;
	  
	av_opt_set_defaults(&pOutVideo->pCodecCtx->priv_data);
	AVDictionary* codec_options=NULL;
	set_opt(&codec_options);
	
	pOutVideo->pCodec = avcodec_find_encoder(AV_CODEC_ID_H264);  
    if (avcodec_open2(pOutVideo->pCodecCtx, pOutVideo->pCodec,&codec_options) < 0)
    {  
        fprintf(stderr,"编码器打开失败！\n");  
        return -1;  
    }
	//av_opt_set(pOutVideo->pCodecCtx->priv_data, "preset","ultrafast", 0);  
	//av_opt_set_defaults(pOutVideo->pCodecCtx->priv_data);
}

int video_combine(char* filelist[],int fileCnt)
{
	char* outfilename=filelist[fileCnt-1];
	videoFile outVideo;
	videoFile inVideo;
	open_video(filelist[1],&inVideo);
	avformat_alloc_output_context2(&(outVideo.pFormatCtx), NULL, NULL, outfilename);
	AVFormatContext* pFormatCtx=outVideo.pFormatCtx;
	init_output(&inVideo,outfilename,&outVideo);
	av_dump_format(outVideo.pFormatCtx,0,outfilename,1);
    if (avio_open(&(pFormatCtx->pb),outfilename, AVIO_FLAG_READ_WRITE) < 0)  
    {  
        printf("输出文件打开失败");
        return -1; 
    }
	
	if(avformat_write_header(pFormatCtx,NULL)<0){fprintf(stderr,"写入文件头失败\n");return -1;}
	SHOW("test point\n");
	printf("开始合并\n");
	int i=0;
	for(i=1;i<fileCnt-1;i++)
	{	
		printf("%s is encoding\n",filelist[i]);
		open_video(filelist[i],&inVideo);
		write_video(&inVideo,&outVideo);
		printf("%s finished encoding\n",filelist[i]);
	}
	if(av_write_trailer(pFormatCtx)!=0){ERROR("写入文件尾失败\n");}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
}*/