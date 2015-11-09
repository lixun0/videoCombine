#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include "video_image.h"
#define BI_RGB 0
void SaveAsBMP (AVFrame *pFrameRGB, int width, int height, int index, int bpp,char* file,int numBytes)
{
 char buf[5] = {0};
 BITMAPFILEHEADER bmpheader;
 BITMAPINFOHEADER bmpinfo;
 printf("%d %d %d %d",sizeof(BITMAPFILEHEADER),sizeof(bmpheader),sizeof(BITMAPINFOHEADER),sizeof(bmpinfo));
 FILE *fp;
 sprintf(buf,"%d",index);
 //itoa (index, buf, 10);
 char filename[50];
 strcpy(filename,file);
 strcat (filename, buf);
 strcat (filename, ".bmp");
 
 if ( (fp=fopen(filename,"wb")) == NULL )
  {
   printf ("open file failed!\n");
   return;
  }
 bmpheader.bfType = 0x4d42;
 bmpheader.bfReserved1 = 0;
 bmpheader.bfReserved2 = 0;
 bmpheader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
 bmpheader.bfSize = bmpheader.bfOffBits + width*height*bpp/8;
 bmpinfo.biSize = sizeof(BITMAPINFOHEADER);
 bmpinfo.biWidth = width;
 bmpinfo.biHeight = -height;
 bmpinfo.biPlanes = 1;
 bmpinfo.biBitCount = bpp;
 bmpinfo.biCompression = BI_RGB;
 bmpinfo.biSizeImage = 0;
 bmpinfo.biXPelsPerMeter = 0;
 bmpinfo.biYPelsPerMeter = 0;
 bmpinfo.biClrUsed = 0;
 bmpinfo.biClrImportant = 0;
 fwrite (&bmpheader, sizeof(bmpheader), 1, fp);
 fwrite (&bmpinfo, sizeof(bmpinfo), 1, fp);
 fwrite (pFrameRGB->data[0], numBytes, 1, fp);
 /*int y=0;
 for(y=height-1; y>=0; y--)
        fwrite(pFrameRGB->data[0]+y*pFrameRGB->linesize[0], 1, width*3, fp);*/
 fclose(fp);
}

int VideoToBMP(videoFile* pvideo,char* filename)
{
	AVFormatContext* pFormatCtx=pvideo->pFormatCtx;
	AVCodecContext* pCodecCtx=pvideo->pCodecCtx;
	int videoStream=pvideo->videoStream;
	//AVCodec* pCodec=video->pCodec;
	AVFrame *pFrame, *pFrameRGB;
	struct SwsContext *pSwsCtx;
	AVPacket packet;
	int frameFinished;
	int PictureSize;
	uint8_t *buf;
	pFrame = avcodec_alloc_frame();
	pFrameRGB = avcodec_alloc_frame();
	if ( (pFrame==NULL)||(pFrameRGB==NULL) )
	{
		printf("avcodec alloc frame failed!\n");
		exit (1);
	}
	PictureSize = avpicture_get_size (PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
	buf = av_malloc(PictureSize*sizeof(uint8_t));
	if ( buf == NULL )
	{
		printf( "av malloc failed!\n");
		exit(1);
	}
	avpicture_fill ( (AVPicture *)pFrameRGB, buf, PIX_FMT_BGR24, pCodecCtx->width, pCodecCtx->height);
	
	pSwsCtx = sws_getContext (pCodecCtx->width,
         pCodecCtx->height,
         pCodecCtx->pix_fmt,
         pCodecCtx->width,
         pCodecCtx->height,
         PIX_FMT_BGR24,
         SWS_BICUBIC,
         NULL, NULL, NULL);
	unsigned int i=0;
	while(av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if(packet.stream_index==videoStream)
		{
			int r=avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
	   
			if(frameFinished&&r>0)
			   {    
				//·´×ªÍ¼Ïñ
				//*(pFrame->data[0]) =  pCodecCtx->width * (pCodecCtx->height-1);
				//pFrame ->linesize[0] = -(pCodecCtx->height);
				/*pFrame->data[0] += pFrame->linesize[0] * (pCodecCtx->height - 1);
				pFrame->linesize[0] *= -1;
				pFrame->data[1] += pFrame->linesize[1] * (pCodecCtx->height / 2 - 1);
				pFrame->linesize[1] *= -1;
				pFrame->data[2] += pFrame->linesize[2] * (pCodecCtx->height / 2 - 1);
				pFrame->linesize[2] *= -1;*/
				sws_scale (pSwsCtx, &pFrame->data, pFrame->linesize, 0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
				SaveAsBMP (pFrameRGB, pCodecCtx->width, pCodecCtx->height, i++, 24,filename,PictureSize);
			   }
		}
		av_free_packet(&packet);
	}
	 sws_freeContext (pSwsCtx);
	 av_free (pFrame);
	 av_free (pFrameRGB);
	 return 0;
}

int VideoToJPG(videoFile* pvideo,char* filename)
{
	AVFormatContext* pFormatCtx=pvideo->pFormatCtx;
	AVCodecContext* pCodecCtx=pvideo->pCodecCtx;
	int videoStream=pvideo->videoStream;
	//AVCodec* pCodec=video->pCodec;
	AVFrame *pFrame;
	AVPacket packet;
	int frameFinished;
	int PictureSize;
	
	pFrame = avcodec_alloc_frame();
	if (pFrame==NULL)
	{
		printf("avcodec alloc frame failed!\n");
		exit (1);
	}
	PictureSize = avpicture_get_size (PIX_FMT_YUVJ420P, pCodecCtx->width, pCodecCtx->height);
	uint8_t *buf = (uint8_t*)av_malloc(PictureSize*sizeof(uint8_t));
	if ( buf == NULL )
	{
		printf( "av malloc failed!\n");
		exit(1);
	}
	
	int i=0;
	while(av_read_frame(pFormatCtx, &packet) >= 0)
	{
		if(packet.stream_index==videoStream)
		{
			int r=avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
	   
			if(frameFinished&&r>0)
			   {
					if ( WriteJPEG(pCodecCtx,pFrame,filename,PIX_FMT_YUVJ420P,buf,PictureSize,i++)!=0)
					{
						printf("write error!\n");
					}
			   }
			else{
				printf("cant get frame\n");
			}
		}
		av_free_packet(&packet);
	}
	
	return 0;
}

int WriteJPEG (AVCodecContext *pCodecCtx, AVFrame *pFrame, char cFileName[],enum PixelFormat pix, uint8_t *buffer, int numBytes,int count)
{
	char filename[50];
	sprintf(filename,"%s%d.jpg",cFileName,count);
   int bRet = 1;
   AVCodec *pMJPEGCodec=NULL;
   AVCodecContext *pMJPEGCtx = avcodec_alloc_context3(NULL);
   if( pMJPEGCtx )
   {
      pMJPEGCtx->bit_rate = pCodecCtx->bit_rate;
      pMJPEGCtx->width = pCodecCtx->width;
      pMJPEGCtx->height = pCodecCtx->height;
      pMJPEGCtx->pix_fmt = pix;
      pMJPEGCtx->codec_id = CODEC_ID_MJPEG;
      pMJPEGCtx->codec_type = AVMEDIA_TYPE_VIDEO;
      pMJPEGCtx->time_base.num = pCodecCtx->time_base.num;
      pMJPEGCtx->time_base.den = pCodecCtx->time_base.den;
      pMJPEGCodec = avcodec_find_encoder(pMJPEGCtx->codec_id );


      if( pMJPEGCodec && (avcodec_open2( pMJPEGCtx, pMJPEGCodec,NULL) >= 0) )
      {
         pMJPEGCtx->qmin = pMJPEGCtx->qmax = 3;
         pMJPEGCtx->mb_lmin = pMJPEGCtx->lmin = pMJPEGCtx->qmin * FF_QP2LAMBDA;
         pMJPEGCtx->mb_lmax = pMJPEGCtx->lmax = pMJPEGCtx->qmax * FF_QP2LAMBDA;
         pMJPEGCtx->flags |= CODEC_FLAG_QSCALE;
         pFrame->quality = 10;
         pFrame->pts = 0;
         int szBufferActual = avcodec_encode_video(pMJPEGCtx, buffer, numBytes, pFrame);
            
         if( SaveFrame(szBufferActual, buffer, filename )==0 )
            bRet = 0;

         avcodec_close(pMJPEGCtx);
      }
   }
   return bRet;
} 

int SaveFrame(int nszBuffer, uint8_t *buffer, char cOutFileName[])
{
//printf("SaveFrame nszBuffer = %d, cOutFileName = %s\n", nszBuffer, cOutFileName);
int bRet = 1;

if( nszBuffer > 0 )
{
FILE *pFile = pFile = fopen(cOutFileName, "wb");
if(pFile)
{
fwrite(buffer, sizeof(uint8_t), nszBuffer, pFile);
bRet = 0;
fclose(pFile);
}
else{printf("%s file open failed\n",cOutFileName);}
}
else {printf("nszBuffer=0\n");}
return bRet;
}