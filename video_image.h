#ifndef VIDEO_IMAGE_H
#define VIDEO_IMAGE_H
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
#include "ffmpeg_decode_encode.h"
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef int LONG;
typedef struct tagBITMAPFILEHEADER {
        WORD    bfType;
        DWORD   bfSize;
        WORD    bfReserved1;
        WORD    bfReserved2;
        DWORD   bfOffBits;
} BITMAPFILEHEADER;
typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER;


int VideoToBMP (videoFile* video,char* filename);
void SaveAsBMP (AVFrame *pFrameRGB, int width, int height, int index, int bpp,char* filename,int numBytes);
int WriteJPEG (AVCodecContext *pCodecCtx, AVFrame *pFrame,char cFileName[],enum PixelFormat pix, uint8_t *buffer, int numBytes,int count);
int SaveFrame(int nszBuffer, uint8_t *buffer, char cOutFileName[]);
int VideoToJPG(videoFile* pVideo,char* filename);
#endif