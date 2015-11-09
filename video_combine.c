/**************
该文件中的代码存在内存泄露问题
为实验方便未进行处理
****************/
#include "video_combine.h"
#include "ffmpeg_decode_encode.h"
#include <libswscale/swscale.h>
#define cal(x,y) x+y-x*y/255

/***********将一个Frame从YUV转成RGB*************/
void YUVToRGB(AVFrame* pFrame)
{
	int height=pFrame->height,width=pFrame->width;
	 AVFrame* pFrameRGB=NULL;  
	 if((pFrameRGB=avcodec_alloc_frame())==NULL){ERROR("alloc fail\n"); } 
      
    int numBytes=avpicture_get_size(PIX_FMT_BGR24,width,height);  
  
    uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));  
  
    // Assign appropriate parts of buffer to image planes in pFrameRGB  
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset  
    // of AVPicture  
	
    avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_BGR24,width, height);
    struct SwsContext *img_convert_ctx=NULL;
	img_convert_ctx=sws_getContext(width,  
        height, PIX_FMT_YUV420P,width, height,  
        PIX_FMT_RGB24, NULL,NULL, NULL, NULL);  
    if( !img_convert_ctx )   
    {  
        ERROR("Cannot initialize sws conversion context");  
    }  
    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data,  
        pFrame->linesize, 0, height, pFrameRGB->data,  
        pFrameRGB->linesize);
	pFrameRGB->height=height;pFrameRGB->width=width;
	/*int i=0;
	for(i=0;i<8;i++){pFrame->data[i]=pFrameRGB->data[i];pFrame->linesize[i]=pFrameRGB->linesize[i];}*/
	avpicture_fill((AVPicture*)pFrame,buffer,PIX_FMT_BGR24,width,height);
}

/***********将一个Frame从RGB转成YUV*************/
void RGBToYUV(AVFrame* pFrame)
{
	int height=pFrame->height,width=pFrame->width;
	 AVFrame* pFrameYUV=NULL;  
	 if((pFrameYUV=avcodec_alloc_frame())==NULL){ERROR("alloc fail\n");} 
      
    int numBytes=avpicture_get_size(PIX_FMT_YUV420P,width,height);  
  
    uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));  
	
    avpicture_fill((AVPicture *)pFrameYUV, buffer, PIX_FMT_YUV420P,width, height);  
    struct SwsContext *img_convert_ctx=NULL;
	img_convert_ctx=sws_getContext(width,  
        height, PIX_FMT_RGB24,width, height,  
        PIX_FMT_YUV420P, NULL,NULL, NULL, NULL);  
    if( !img_convert_ctx )   
    {  
        ERROR("Cannot initialize sws conversion context");  
    }  
    
    sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data,  
        pFrame->linesize, 0, height, pFrameYUV->data,  
        pFrameYUV->linesize);  
    pFrameYUV->height=height;pFrameYUV->width=width; 
	/*int i=0;
	for(i=0;i<8;i++){pFrame->data[i]=pFrameYUV->data[i];pFrame->linesize[i]=pFrameYUV->linesize[i];}*/
	avpicture_fill((AVPicture*)pFrame,buffer,PIX_FMT_YUV420P,width,height);
}

/******************将两个帧合并成一个帧返回**********************/
AVFrame* frame_combine(AVFrame* pFrame1,const AVFrame* pFrame2)
{
	YUVToRGB(pFrame1),YUVToRGB(pFrame2);
	AVFrame* r=pFrame1;
	AVFrame *f1=pFrame1,*f2=pFrame2;
	
	int i,j;
	for(i=0;i<f1->height;i++)
	{
		for(j=0;j<3*f1->width;j++)
		{
			int index=i*r->width*3+j;
			int a=f1->data[0][index],b=f2->data[0][index];
			r->data[0][index]=cal(a,b);
		}
	}
	RGBToYUV(r);
	return r;
}

/********************将一个YUV帧复制到另一个YUV帧中****************/
void frameCpy(AVFrame** dest,const AVFrame* src)
{
	int width=src->width,height=src->height;
	//(*dest)=(AVFrame*)malloc(sizeof(AVFrame));
	(*dest)=avcodec_alloc_frame();
	//printf("%d\n",sizeof(AVFrame));
	(*dest)->width=width;
	(*dest)->height=height;
	(**dest)=(*src);
	(*dest)->data[0]=malloc(src->linesize[0]*height);
	(*dest)->data[1]=malloc(src->linesize[1]*height/2);
	(*dest)->data[2]=malloc(src->linesize[2]*height/2);
	//int numBytes=avpicture_get_size(PIX_FMT_YUV420P,width,height);  
    //uint8_t * buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));  
    //avpicture_fill((AVPicture *)(*dest), src->data, PIX_FMT_YUV420P,width, height);
	int i=0,j=0;
	for(i=0;i<3;i++)
	{
		(*dest)->linesize[i]=src->linesize[i];
		if(i==0){memcpy((*dest)->data[i],src->data[i],height*src->linesize[i]);}
		else{memcpy((*dest)->data[i],src->data[i],height/2*src->linesize[i]);}
	}
}
static AVFrame* frameArr[10000];
static int video[100];

int read_video(const videoFile* inVideo,AVFrame* frameArr[],int start)
{
	/******************初始化*******************************/
	AVPacket pkt;  
	int frameFinished=0;
	int i=start;
	int got_picture=0;
	AVFrame* picture = avcodec_alloc_frame();
	if (picture==NULL)
	{
		printf("avcodec alloc frame failed!\n");
		exit (1);
	}
	while(av_read_frame(inVideo->pFormatCtx,&pkt)>=0)
	{
		if(pkt.stream_index==inVideo->videoStream)
		{
			int r=avcodec_decode_video2(inVideo->pCodecCtx, picture, &frameFinished, &pkt);
			if(frameFinished&&r>=0)
			{
				picture->pts=pkt.pts;
				frameCpy(&frameArr[i++],picture);
			}
		}
		//av_free_packet(&pkt);
	}
	/*********************释放内存****************************/
    av_free(picture);  
	return i-start;
}

int write_video3(videoFile* outVideo,videoFile* inVideo,AVFrame* frameArr[],int* index,int videoCnt)
{
	AVCodecContext* pCodecCtx=outVideo->pCodecCtx;
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);  
	AVStream* video_st = outVideo->video_st;  
    if (!pCodec){fprintf(stderr,"没有找到合适的编码器！\n");return -1;}  
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){fprintf(stderr,"编码器打开失败！\n");return -1;}  
	
	
	uint64_t tmpPTS=0;
	static uint64_t PTS=0;
	int len=index[videoCnt],i=0;
	int got_picture=0;
	
	#define COMCNT 25

	for(i=0;i<=len;i++)
	{
		int j;
		for(j=1;j<videoCnt;j++)
		{
			if(i==index[j]+1)i+=COMCNT;
			if(i<=index[j]&&i>index[j]-COMCNT)
			{
				frameArr[i]=frame_combine(frameArr[i+COMCNT],frameArr[i]);
			}
		}
		//printf("%d\n",i);
		AVFrame* picture=frameArr[i];
		AVPacket inPKT;
		int y_size = pCodecCtx->width * pCodecCtx->height;
		av_new_packet(&inPKT,y_size*3); 
		picture->pts=inPKT.pts;
		if(avcodec_encode_video2(pCodecCtx, &inPKT,picture, &got_picture)>=0)
		{
			
			(picture->pts)++;
			inPKT.pts+=PTS;
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
		}
		av_write_frame(outVideo->pFormatCtx, &inPKT);
	}
	/*********************编码延迟帧****************************/
	for(got_picture=1;got_picture;)
	{
		AVPacket inPKT;
		int y_size = pCodecCtx->width * pCodecCtx->height;
		av_new_packet(&inPKT,y_size);
		if(avcodec_encode_video2(pCodecCtx, &inPKT,NULL, &got_picture)>=0)
		{
			inPKT.pts+=PTS;
			tmpPTS=inPKT.pts;
			if(got_picture!=1){break;}
			
			if (got_picture != 0) {
				if (inPKT.pts != AV_NOPTS_VALUE) {
					inPKT.pts = av_rescale_q(inPKT.pts, inVideo->pCodecCtx->time_base,video_st->time_base);
					}
				if (inPKT.dts != AV_NOPTS_VALUE) {
					inPKT.dts = av_rescale_q(inPKT.dts, inVideo->pCodecCtx->time_base,video_st->time_base);
					}
					inPKT.stream_index = video_st->index;
				}
		av_write_frame(outVideo->pFormatCtx, &inPKT);  
		}
		av_free_packet(&inPKT); 
	}
	PTS=tmpPTS;
	return 0;
}

/*************************将一个视频中的帧以append方式写入另一个视频文件中*********/
int write_video2(const videoFile* inVideo,videoFile* outVideo)
{
	/******************初始化*******************************/
	static int videoCnt=0;
	
	AVCodecContext* pCodecCtx=outVideo->pCodecCtx;
	AVCodec* pCodec = avcodec_find_encoder(pCodecCtx->codec_id);  
	AVStream* video_st = outVideo->video_st;  
    if (!pCodec){fprintf(stderr,"没有找到合适的编码器！\n");return -1;}  
    if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){fprintf(stderr,"编码器打开失败！\n");return -1;}  
	
	AVPacket pkt;  
	int frameFinished=0;
	static int i=0;
	uint64_t tmpPTS=0;
	static uint64_t PTS=0;
	int j=0;
	int got_picture=0;
	AVFrame* picture = avcodec_alloc_frame();
	if (picture==NULL)
	{
		printf("avcodec alloc frame failed!\n");
		exit (1);
	}
	/*****************开始解码和重编码******************/
	while(av_read_frame(inVideo->pFormatCtx,&pkt)>=0)
	{
		
		if(pkt.stream_index==inVideo->videoStream)
		{
			
			i++;
			int r=avcodec_decode_video2(inVideo->pCodecCtx, picture, &frameFinished, &pkt);
			
			if(frameFinished&&r>=0)
			{
					if(picture->format!=PIX_FMT_YUV420P){ERROR("format error");exit(1);}
					AVPacket inPKT;
					int y_size = pCodecCtx->width * pCodecCtx->height;
					av_new_packet(&inPKT,y_size);

					/****************test****************************************/

					if(videoCnt==0){
					//frameArr[j++]=picture;
					//printf("%d %d\n",picture->width,picture->height);
					frameCpy(&frameArr[j++],picture);
					//printf("%d %d\n",frameArr[j-1]->width,frameArr[j-1]->height);
					}
					if(videoCnt>0){picture=frame_combine(picture,frameArr[j++]);
					}

					/***********************************************************/
					
					picture->pts=inPKT.pts;
					
					if(avcodec_encode_video2(pCodecCtx, &inPKT,picture, &got_picture)>=0)
					{
						(picture->pts)++;
						inPKT.pts+=PTS;
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
	/*********************编码延迟帧****************************/
	for(got_picture=1;got_picture;)
	{
		AVPacket inPKT;
		int y_size = pCodecCtx->width * pCodecCtx->height;
		av_new_packet(&inPKT,y_size);
		if(avcodec_encode_video2(pCodecCtx, &inPKT,NULL, &got_picture)>=0)
		{
			inPKT.pts+=PTS;
			tmpPTS=inPKT.pts;
			if(got_picture!=1){break;}
			
			if (got_picture != 0) {
				if (inPKT.pts != AV_NOPTS_VALUE) {
					inPKT.pts = av_rescale_q(inPKT.pts, inVideo->pCodecCtx->time_base,video_st->time_base);
					}
				if (inPKT.dts != AV_NOPTS_VALUE) {
					inPKT.dts = av_rescale_q(inPKT.dts, inVideo->pCodecCtx->time_base,video_st->time_base);
					}
					inPKT.stream_index = video_st->index;
				}
		av_write_frame(outVideo->pFormatCtx, &inPKT);  
		}
		av_free_packet(&inPKT); 
	}
	PTS=tmpPTS;
	/*********************释放内存****************************/
	
	if (video_st)  
    {  
        avcodec_close(video_st->codec);  
        av_free(picture);  
    }  
	//printf("写入完成\n");
	videoCnt+=1;
	return 0;
}

/*设置输出视频的各类参数*/
void set_opt(AVDictionary** codec_options)
{
	av_dict_set(codec_options,"preset","ultrafast",0);
	//av_dict_set(codec_options,"qpmin","0",0);
	//av_dict_set(codec_options,"qpmax","69",0);
	av_dict_set(codec_options,"vcodec","libx264",0);
	av_dict_set(codec_options,"tune","zerolatency",0);
	av_dict_set(codec_options,"threads","1",0);
	av_dict_set(codec_options,"b-adapt","0",0);
	//av_dict_set(codec_options,"me_range","16",0);
	av_dict_set(codec_options,"qcomp","0",0);
	//av_dict_set(codec_options,"profile","baseline",0);
	//av_dict_set(codec_options,"fast_pskip","1",0);
	//av_dict_set(codec_options,"open_gop","0",0);
	//av_dict_set(codec_options,"qpstep","4",0);
	//av_dict_set(codec_options,"ip_ratio","1.4",0);
	//av_dict_set(codec_options,"aq","0",0);
}
/*************************初始化输出视频*************************/
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
	//pOutVideo->pCodecCtx->qcompress = 0;  
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
	
	int i=0;
	for(i=1;i<fileCnt-1;i++)
	{	
		open_video(filelist[i],&inVideo);
		write_video2(&inVideo,&outVideo);
	}
	
	if(av_write_trailer(pFormatCtx)!=0){ERROR("写入文件尾失败\n");}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
	return 0;
}
/*视频合成主调用*/
int video_combine2(char* filelist[],int fileCnt)
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
	
	if(avformat_write_header(pFormatCtx,NULL)<0){ERROR("写入文件尾失败");}
	AVFrame* frameArr[10000];
	int index[10]={0};
	int i=0;
	for(i=1;i<fileCnt-1;i++)
	{	
		open_video(filelist[i],&inVideo);
		//printf("%d\n",read_video(&inVideo,frameArr,index[i-1]));
		if(i==1){index[i]=read_video(&inVideo,frameArr,0)-1;}
		else{index[i]=read_video(&inVideo,frameArr,index[i-1]+1)+index[i-1];}
	}
	//STOP;
	//printf("%d %d %d %d\n",index[0],index[1],index[2],index[3]);
	
	write_video3(&outVideo,&inVideo,frameArr,index,fileCnt-2);
	
	if(av_write_trailer(pFormatCtx)!=0){ERROR("写入文件尾失败");}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
}