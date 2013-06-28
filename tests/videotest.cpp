/*
 * videotest.cpp
 *
 *  Created on: Jun 27, 2013
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
#include <time.h>
#include <stdio.h>

int load_video(const char* path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int *videostream);
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff);

int main(int argc, char *argv[]) {

	AVFormatContext *pFormatCtx1, *pFormatCtx2 = NULL, *pFormatCtx3 = NULL, *pFormatCtx4 = NULL,
					*pFormatCtx5 = NULL, *pFormatCtx6 = NULL, *pFormatCtx7 = NULL, *pFormatCtx8 = NULL;
	int             videostream1, videostream2, videostream3, videostream4, videostream5, videostream6, videostream7, videostream8;
	AVCodecContext  pCodecCtx1, pCodecCtx2, pCodecCtx3, pCodecCtx4,	pCodecCtx5, pCodecCtx6, pCodecCtx7, pCodecCtx8;
	uint8_t 		*b1=NULL, *b2=NULL, *b3=NULL, *b4=NULL, *b5=NULL, *b6=NULL, *b7=NULL, *b8=NULL;
	int 			ret1=0, ret2=0, ret3=0, ret4=0, ret5=0, ret6=0, ret7=0, ret8=0, frame_cont=0;
	clock_t 		begin, end, begin_reading, end_reading;
	float 			elapsed_secs=0, reading_secs=0;

	const char* path1 = "/home/palau/Videos/ka20s.yuv";
	const char* path2 = "/home/palau/Videos/ka20s.yuv";
	const char* path3 = "/home/palau/Videos/ka20s.yuv";
	const char* path4 = "/home/palau/Videos/ka20s.yuv";
	const char* path5 = "/home/palau/Videos/ka20s.yuv";
	const char* path6 = "/home/palau/Videos/ka20s.yuv";
	const char* path7 = "/home/palau/Videos/ka20s.yuv";
	const char* path8 = "/home/palau/Videos/ka20s.yuv";

	// Register all formats and codecs
	av_register_all();

	pFormatCtx1 = avformat_alloc_context();
	pFormatCtx2 = avformat_alloc_context();
	pFormatCtx3 = avformat_alloc_context();
	pFormatCtx4 = avformat_alloc_context();
	pFormatCtx5 = avformat_alloc_context();
	pFormatCtx6 = avformat_alloc_context();
	pFormatCtx7 = avformat_alloc_context();
	pFormatCtx8 = avformat_alloc_context();

	//Load videos
	load_video(path1, pFormatCtx1, &pCodecCtx1, &videostream1);
	load_video(path2, pFormatCtx2, &pCodecCtx2, &videostream2);
	load_video(path3, pFormatCtx3, &pCodecCtx3, &videostream3);
	load_video(path4, pFormatCtx4, &pCodecCtx4, &videostream4);
	load_video(path5, pFormatCtx5, &pCodecCtx5, &videostream5);
	load_video(path6, pFormatCtx6, &pCodecCtx6, &videostream6);
	load_video(path7, pFormatCtx7, &pCodecCtx7, &videostream7);
	load_video(path8, pFormatCtx8, &pCodecCtx8, &videostream8);

	b1=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx1.pix_fmt, pCodecCtx1.width, pCodecCtx1.height)*sizeof(uint8_t));
	b2=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx2.pix_fmt, pCodecCtx2.width, pCodecCtx2.height)*sizeof(uint8_t));
	b3=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx3.pix_fmt, pCodecCtx3.width, pCodecCtx3.height)*sizeof(uint8_t));
	b4=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx4.pix_fmt, pCodecCtx4.width, pCodecCtx4.height)*sizeof(uint8_t));
	b5=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx5.pix_fmt, pCodecCtx5.width, pCodecCtx5.height)*sizeof(uint8_t));
	b6=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx6.pix_fmt, pCodecCtx6.width, pCodecCtx6.height)*sizeof(uint8_t));
	b7=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx7.pix_fmt, pCodecCtx7.width, pCodecCtx7.height)*sizeof(uint8_t));
	b8=(uint8_t *)av_malloc(avpicture_get_size(pCodecCtx8.pix_fmt, pCodecCtx8.width, pCodecCtx8.height)*sizeof(uint8_t));

	//Init layout
	Layout layout;
	layout.init(1920, 1080, PIX_FMT_RGB24, 8);
	printf ("Initializing layout 1920x1080 and 8 streams...\n");

	begin = clock();

	//Introduce streams
	int id1 = layout.introduce_stream(pCodecCtx1.width, pCodecCtx1.height, pCodecCtx1.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id2 = layout.introduce_stream(pCodecCtx2.width, pCodecCtx2.height, pCodecCtx2.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id3 = layout.introduce_stream(pCodecCtx3.width, pCodecCtx2.height, pCodecCtx3.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id4 = layout.introduce_stream(pCodecCtx4.width, pCodecCtx4.height, pCodecCtx4.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id5 = layout.introduce_stream(pCodecCtx5.width, pCodecCtx5.height, pCodecCtx5.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id6 = layout.introduce_stream(pCodecCtx6.width, pCodecCtx6.height, pCodecCtx6.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id7 = layout.introduce_stream(pCodecCtx7.width, pCodecCtx7.height, pCodecCtx7.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id8 = layout.introduce_stream(pCodecCtx8.width, pCodecCtx8.height, pCodecCtx8.pix_fmt, 720, 480, 0, 0, PIX_FMT_RGB24, 0);

    // Main loop: read frame, resize if necessary, fill the mixed frame and save to disk
    while(1){

    	begin_reading = clock();

    	ret1 = read_frame(pFormatCtx1, videostream1, &pCodecCtx1, b1);
//    	ret2 = read_frame(pFormatCtx2, videostream2, &pCodecCtx2, b2);
//    	ret3 = read_frame(pFormatCtx3, videostream3, &pCodecCtx3, b3);
//    	ret4 = read_frame(pFormatCtx4, videostream4, &pCodecCtx4, b4);
//    	ret5 = read_frame(pFormatCtx5, videostream5, &pCodecCtx5, b5);
//    	ret6 = read_frame(pFormatCtx6, videostream6, &pCodecCtx6, b6);
//    	ret7 = read_frame(pFormatCtx7, videostream7, &pCodecCtx7, b7);
//    	ret8 = read_frame(pFormatCtx8, videostream8, &pCodecCtx8, b8);

    	end_reading = clock();

    	if (ret1<0 || ret2<0 || ret3<0 || ret4<0 || ret5<0 || ret6<0 || ret7<0 || ret8<0){
    		break;
    	}

    	layout.introduce_frame(id1, b1);
//    	layout.introduce_frame(id2, b2);
//    	layout.introduce_frame(id3, b3);
//    	layout.introduce_frame(id4, b4);
//    	layout.introduce_frame(id5, b5);
//    	layout.introduce_frame(id6, b6);
//    	layout.introduce_frame(id7, b7);
//    	layout.introduce_frame(id8, b8);

    	layout.merge_frames();

//    	SaveFrame(layout.get_lay_frame(), layout.get_w(), layout.get_h(), frame_cont);

        reading_secs += (((float)end_reading)-((float)begin_reading))/CLOCKS_PER_SEC;

    	printf("Frame: %d\n", frame_cont++);

    	if(frame_cont>400){
    			break;
    		}
    	}


    end = clock();

    elapsed_secs = ((((float)end)-((float)begin))/CLOCKS_PER_SEC) - reading_secs;

    printf("Elapsed time: %f",elapsed_secs);


    return 0;
}

int load_video(const char* path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int *videostream){

	AVDictionary *rawdict = NULL, *optionsDict = NULL;
	AVCodec *pCodec = NULL;
	AVCodecContext *aux_codec_ctx = NULL;

	// Define YUV input video features
	pFormatCtx->iformat = av_find_input_format("rawvideo");
	int i;

	av_dict_set(&rawdict, "video_size", "720x480", 0);
	av_dict_set(&rawdict, "pixel_format", "yuv420p", 0);

	// Open video file
	if(avformat_open_input(&pFormatCtx, path, pFormatCtx->iformat, &rawdict)!=0)
		return -1; // Couldn't open file

	av_dict_free(&rawdict);

	// Retrieve stream information
	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
		return -1; // Couldn't find stream information

	// Find the first video stream
	*videostream=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++){
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
			*videostream=i;
			break;
		}
	}
	if(*videostream==-1)
		return -1; // Didn't find a video stream

	// Get a pointer to the codec context for the video stream
	aux_codec_ctx=pFormatCtx->streams[*videostream]->codec;

	// Find the decoder for the video stream
	pCodec=avcodec_find_decoder(aux_codec_ctx->codec_id);
	if(pCodec==NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}

	// Open codec
	if(avcodec_open2(aux_codec_ctx, pCodec, &optionsDict)<0)
		return -1; // Could not open codec

	*pCodecCtx = *aux_codec_ctx;

	return 0;

}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[64];
  int  y;

  // Open file
  sprintf(szFilename, "/home/palau/TFG/layout_prints/3/layout%d.ppm", iFrame);
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

int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff){
	AVPacket packet;
	AVFrame* pFrame;
	int frameFinished, ret;

	pFrame = avcodec_alloc_frame();
	ret = av_read_frame(pFormatCtx, &packet);

	if(packet.stream_index==videostream) {
		// Decode video frame
		avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
		// Did we get a video frame?
		if(frameFinished) {
			avpicture_layout((AVPicture *)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, buff,
					avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height)*sizeof(uint8_t));

			// Free the packet that was allocated by av_read_frame
			av_free_packet(&packet);
		}
	}

	return ret;
}
