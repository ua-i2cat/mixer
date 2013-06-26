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

uint8_t* load_image(AVFrame* out_frame, enum AVPixelFormat *cp, int *w, int *h, const char* path);
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

int main ()
{
	//Prepare different test images
	AVFrame *im1=NULL, *im2=NULL, *im3=NULL, *im4=NULL, *im5=NULL, *im6=NULL, *im7=NULL, *im8=NULL;
	uint8_t *b1=NULL, *b2=NULL, *b3=NULL, *b4=NULL, *b5=NULL, *b6=NULL, *b7=NULL, *b8=NULL;
	int w1=0, w2=0, w3=0, w4=0, w5=0, w6=0, w7=0, w8=0;
	int h1=0, h2=0, h3=0, h4=0, h5=0, h6=0, h7=0, h8=0;
	enum AVPixelFormat cp1, cp2, cp3, cp4, cp5, cp6, cp7, cp8;

	const char* path1 = "/home/palau/Pictures/baboon.jpg";
	const char* path2 = "/home/palau/Pictures/lena.jpg";
	const char* path3 = "/home/palau/Pictures/foreman.jpg";
	const char* path4 = "/home/palau/Pictures/cameraman.jpg";
	const char* path5 = "/home/palau/Pictures/baboon.jpg";
	const char* path6 = "/home/palau/Pictures/lena.jpg";
	const char* path7 = "/home/palau/Pictures/foreman.jpg";
	const char* path8 = "/home/palau/Pictures/cameraman.jpg";

	im1=avcodec_alloc_frame();
	im2=avcodec_alloc_frame();
	im3=avcodec_alloc_frame();
	im4=avcodec_alloc_frame();
	im5=avcodec_alloc_frame();
	im6=avcodec_alloc_frame();
	im7=avcodec_alloc_frame();
	im8=avcodec_alloc_frame();

	b1 = load_image(im1, &cp1, &w1, &h1, path1);
	b2 = load_image(im2, &cp2, &w2, &h2, path2);
	b3 = load_image(im3, &cp3, &w3, &h3, path3);
	b4 = load_image(im4, &cp4, &w4, &h4, path4);
	b5 = load_image(im5, &cp5, &w5, &h5, path5);
	b6 = load_image(im6, &cp6, &w6, &h6, path6);
	b7 = load_image(im7, &cp7, &w7, &h7, path7);
	b8 = load_image(im8, &cp8, &w8, &h8, path8);

	Layout layout;
	layout.init(1920, 1080, PIX_FMT_RGB24, 8);
	printf ("Initializing layout 1920x1080 and 8 streams...\n");

	int option;
	int num_frame = 1;
	while (1){
		printf("Please enter a Layout function:\n");
		printf("1 - Introduce stream\n");
		printf("2 - introduce frame\n");
		printf("3 - Merge frames\n");
		printf("4 - Modify stream\n");
		printf("5 - Remove stream\n");
		printf("6 - Modify layout\n");
		printf("7 - Get layout bytestream\n");
		printf("8 - Save layout to image\n");

	scanf ("%d",&option);

	int orig_w, orig_h, new_w, new_h, x, y, stream_id, layer, keepratio, resize, cont=0, numstreams=0;
	uint8_t** layout_matrix = NULL;
	bool keepAspectRatio = false, resizeStreams = false;
	AVPixelFormat orig_cp, new_cp;
	switch(option){
	case 1:
		printf("You have choosen the %d option: Introduce stream\n", option);
		printf("Please introduce the values this way: new_w new_h position_x position_y new_cp and layer\n");
		scanf("%d %d %d %d %d %d", &new_w, &new_h, &x, &y, &new_cp, &layer);
		numstreams= layout.get_active_streams() + 1;
		switch (numstreams){
				case 1:
					if (layout.introduce_stream(w1, h1, cp1, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 2:
					if (layout.introduce_stream(w2, h2, cp2, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 3:
					if (layout.introduce_stream(w3, h3, cp3, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 4:
					if (layout.introduce_stream(w4, h4, cp4, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 5:
					if (layout.introduce_stream(w5, h5, cp5, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 6:
					if (layout.introduce_stream(w6, h6, cp6, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 7:
					if (layout.introduce_stream(w7, h7, cp7, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				case 8:
					if (layout.introduce_stream(w8, h8, cp8, new_w, new_h, x, y, new_cp, layer) == -1)
							printf ("Error while entering the strem\n");
					break;
				}
		break;
	case 2:
		printf("You have choosen the %d option: Introduce frame\n", option);
		printf("Please introduce the values this way: stream_id\n");
		scanf("%d", &stream_id);
		switch (stream_id){
		case 0:
			if (layout.introduce_frame(stream_id, b1) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 1:
			if (layout.introduce_frame(stream_id,b2) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 2:
			if (layout.introduce_frame(stream_id, b3) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 3:
			if (layout.introduce_frame(stream_id, b4) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 4:
			if (layout.introduce_frame(stream_id, b5) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 5:
			if (layout.introduce_frame(stream_id, b6) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 6:
			if (layout.introduce_frame(stream_id,b7) == -1)
				printf ("Error while entering the frame\n");
			break;
		case 7:
			if (layout.introduce_frame(stream_id,b8) == -1)
				printf ("Error while entering the frame\n");
			break;
		}
		
		break;
	case 3:
		printf("You have choosen the %d option: Merge frames\n", option);
		if (layout.merge_frames() == -1)
			printf ("Error while merging the frames\n");
		break;
	case 4:
		printf("You have choosen the %d option: Modify stream\n", option);
		printf("Please introduce the values this way: stream_id width heigth colorspace position_x position_y layer and keepAspectRatio?\n");		
		scanf("%d %d %d %d %d %d %d %d", &stream_id,  &new_w, &new_h, &new_cp, &x, &y, &layer, &keepratio);
		if (keepratio == 1)
			keepAspectRatio = true;
		if (layout.modify_stream(stream_id, new_w, new_h, new_cp, x, y, layer, keepAspectRatio) == -1)
			printf ("Error while modifying the stream\n");
		break;
	case 5:
		printf("You have choosen the %d option: Remove stream\n", option);
		printf("Please introduce the values this way: stream_id\n");
		scanf("%d", &stream_id);
		if (layout.remove_stream(stream_id) == -1)
			printf ("Error deleting the stream %d\n", stream_id);
		break;
	case 6:
		printf("You have choosen the %d option: Modify layout\n", option);
		printf("Please introduce the values this way: width heigth colorspace and resizestreams?\n");		
		scanf("%d %d %d %d ", &new_w, &new_h, &new_cp, &resize);
		if (resize == 1)
			resizeStreams = true;
		if (layout.modify_layout(new_w, new_h, new_cp, resizeStreams) == -1)
			printf ("Error while resizing the layout\n");
		break;
	case 7:
		printf("You have choosen the %d option: Get layout bytestream\n", option);
		if ((layout_matrix = layout.get_layout_bytestream() ) != NULL)
			printf ("The layout is empty\n");
		break;
	case 8:
		printf("You have choosen the %d option: Save layout to a image\n", option);
		cont++;
		SaveFrame(layout.get_lay_frame(), layout.get_w(), layout.get_h(), cont);
			break;
	default:
		cout << "Incorrect option. Only 1 to 7 are correct.\n";
		break;
	}
	}
	return 0;
}

uint8_t* load_image(AVFrame* out_frame, enum AVPixelFormat *cp, int *w, int *h, const char* path) {
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream, frameFinished;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVPacket        packet;
  AVDictionary    *optionsDict = NULL;


  // Register all formats and codecs
  av_register_all();

  // Open video file
  if(avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0)
    //return -1; // Couldn't open file

  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return NULL; // Couldn't find stream information

  // Find the first video stream
  videoStream=-1;
  for(i=0; i<(int)pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return NULL; // Didn't find a video stream

  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;

  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return NULL;// Codec not found
  }
  // Open codec
  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return NULL; // Could not open codec

  // Allocate video frame
 // out_frame=avcodec_alloc_frame();

  // Read frames and save first five frames to disk
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream) {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, out_frame, &frameFinished,
			   &packet);

    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
  }
    //out_frame = pFrame;
    *cp = pCodecCtx->pix_fmt;
    *w = pCodecCtx->width;
    *h = pCodecCtx->height;

  }
  int numBytes = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
      uint8_t *buff1=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
      avpicture_layout((AVPicture *)out_frame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, buff1, numBytes*sizeof(uint8_t));
  //av_free(pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return buff1;
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;

  // Open file
  sprintf(szFilename, "/home/palau/Pictures/layout%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

  // Close file
  fclose(pFile);
}


