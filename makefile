HOME=/data/enzoli
LIB_PATH=$(HOME)/usr/local/lib
I_PATH = $(HOME)/usr/local/include

INCLUDE = -I$(I_PATH)
# LIB = $(LIB_PATH)/libavcodec.a \
      # $(LIB_PATH)/libavformat.a \
      # $(LIB_PATH)/libavutil.a \
      # $(LIB_PATH)/libavcodec.so \
      # $(LIB_PATH)/libavformat.so \
      # $(LIB_PATH)/libavutil.so \
	  # $(LIB_PATH)/libavdevice.a \
	  # $(LIB_PATH)/libavdevice.so 
CFLAGS = -L$(LIB_PATH) -I$(I_PATH)
LIB= -lavcodec -lavformat -lavutil -lavdevice -lx264

GCC=gcc

test : test.o ffmpeg_decode_encode.o video_image.o video_combine.o
	$(GCC) $(CFLAGS) $(LIB) -o $@ $^

%.o : %.c
	$(GCC) $(CFLAGS) -o $@ -c $^

clean : 
	rm -f *.o