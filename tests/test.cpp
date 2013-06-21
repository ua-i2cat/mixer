/*
 * test.cpp
 *
 *  Created on: Jun 20, 2013
 *      Author: palau
 */
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
	#include <libavdevice/avdevice.h>
}
#include <iostream>
#include "layout.h"

using namespace std;

int load_image(AVFrame* out_frame, enum AVPixelFormat *cp, int *w, int *h, const char* path);

int main ()
{
	//Prepare different test images
	AVFrame *im1=NULL, *im2=NULL, *im3=NULL, *im4=NULL;
	int w1=0, w2=0, w3=0, w4=0;
	int h1=0, h2=0, h3=0, h4=0;
	enum AVPixelFormat cp1, cp2, cp3, cp4;
	const char* path1 = "/home/palau/Pictures/baboon.jpg";
	const char* path2 = "/home/palau/Pictures/lena.jpg";
	const char* path3 = "/home/palau/Pictures/foreman.jpg";
	const char* path4 = "/home/palau/Pictures/cameraman.jpg";

	load_image(im1, &cp1, &w1, &h1, path1);
	load_image(im2, &cp2, &w2, &h2, path2);
	load_image(im3, &cp3, &w3, &h3, path3);
	load_image(im4, &cp4, &w4, &h4, path4);

	Layout layout;
	layout.init(1920, 1080, PIX_FMT_RGB24, 4);

	int i;
	while (1){
		printf("Please enter a Layout function:\n");
		printf("1 - Introduce stream\n");
		printf("2 - introduce frame\n");
		printf("3 - Merge frames\n");
		printf("4 - Modify stream\n");
		printf("5 - Remove stream\n");
		printf("6 - Modify layout\n");
		printf("7 - Get layout bytestream\n");

	scanf ("%d",&i);;
	switch(i){
	case 1:
		int orig_w, orig_h, new_w, new_h, x, y;
		AVPixelFormat orig_cp, new_cp;
		printf("You have choosen the %d option: Introduce stream\n", i);
		printf("Please introduce the values this way: orig_w orig_h orig_cp new_w new_h x y new_cp\n");
		scanf("%d %d %d %d %d %d %d %d", &orig_w, &orig_h, &orig_cp, &new_w, &new_h, &x, &y, &new_cp);
		layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	case 2:
		printf("You have choosen the %d option: Introduce frame\n", i);
		//layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	case 3:
		printf("You have choosen the %d option: Merge frames\n", i);
		//layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	case 4:
		printf("You have choosen the %d option: Modify stream\n", i);
		//layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	case 5:
		printf("You have choosen the %d option: Remove stream\n", i);
		//layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	case 6:
		printf("You have choosen the %d option: Modify layout\n", i);
		//layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	case 7:
		printf("You have choosen the %d option: Get layout bytestream\n", i);
		//layout.introduce_stream(orig_w, orig_h, orig_cp, new_w, new_h, x, y, new_cp);
		break;
	default:
		cout << "Incorrect option. Only 1 to 7 are correct.\n";
		break;
	}
	}
	return 0;
}

int load_image(AVFrame* out_frame, enum AVPixelFormat *cp, int *w, int *h, const char* path) {
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream, frameFinished;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL;
  AVPacket        packet;
  AVDictionary    *optionsDict = NULL;


  // Register all formats and codecs
  av_register_all();

  // Open video file
  if(avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0)
    //return -1; // Couldn't open file

  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information

  // Find the first video stream
  videoStream=-1;
  for(i=0; i<(int)pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1; // Didn't find a video stream

  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;

  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1;// Codec not found
  }
  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return -1; // Could not open codec

  // Allocate video frame
  pFrame=avcodec_alloc_frame();

  // Read frames and save first five frames to disk
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished,
			   &packet);

    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
  }

    out_frame = pFrame;
    *cp = pCodecCtx->pix_fmt;
    *w = pCodecCtx->width;
    *h = pCodecCtx->height;


  }
  av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}


