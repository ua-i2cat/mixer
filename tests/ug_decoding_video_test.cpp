/*
 * ug_decoding_video_test.cpp
 *
 *  Created on: Jul 4, 2013
 *      Author: palau
 */

//extern "C" {
//	#include <libavcodec/avcodec.h>
//	#include <libavformat/avformat.h>
//	#include <libswscale/swscale.h>
//	#include <libavutil/avutil.h>
//	#include <libavdevice/avdevice.h>
//}
//#include <iostream>
//#include "layout.h"
//#include <time.h>
//#include <stdio.h>
//#include <string.h>
//#include "video_decompress.h"
//#include "video_decompress/libavcodec.h"
//
//int load_video(const char* path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int *videostream);
//void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);
//int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff, struct state_decompress *sd, int frame_cont);
//void * init_decoder(AVCodecContext *pCodecCtx);
//
//int main(int argc, char *argv[]) {
//
//	AVFormatContext *pFormatCtx1, *pFormatCtx2 = NULL, *pFormatCtx3 = NULL, *pFormatCtx4 = NULL,
//					*pFormatCtx5 = NULL, *pFormatCtx6 = NULL, *pFormatCtx7 = NULL, *pFormatCtx8 = NULL;
//	int             videostream1, videostream2, videostream3, videostream4, videostream5, videostream6, videostream7, videostream8;
//	AVCodecContext  pCodecCtx1, pCodecCtx2, pCodecCtx3, pCodecCtx4,	pCodecCtx5, pCodecCtx6, pCodecCtx7, pCodecCtx8;
//	uint8_t 		*b1=NULL, *b2=NULL, *b3=NULL, *b4=NULL, *b5=NULL, *b6=NULL, *b7=NULL, *b8=NULL;
//	int 			ret1=0, ret2=0, ret3=0, ret4=0, ret5=0, ret6=0, ret7=0, ret8=0, frame_cont=0;
//	struct state_decompress *sd1, *sd2, *sd3, *sd4, *sd5, *sd6, *sd7, *sd8;
//	clock_t 		begin, end, begin_reading, end_reading;
//	float 			elapsed_secs=0, reading_secs=0;
//
//	const char* path1 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path2 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path3 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path4 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path5 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path6 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path7 = "/home/palau/Videos/sintel-1024-surround.mp4";
//	const char* path8 = "/home/palau/Videos/sintel-1024-surround.mp4";
//
//	pFormatCtx1 = avformat_alloc_context();
//	pFormatCtx2 = avformat_alloc_context();
////	pFormatCtx3 = avformat_alloc_context();
////	pFormatCtx4 = avformat_alloc_context();
////	pFormatCtx5 = avformat_alloc_context();
////	pFormatCtx6 = avformat_alloc_context();
////	pFormatCtx7 = avformat_alloc_context();
////	pFormatCtx8 = avformat_alloc_context();
//
//	av_register_all();
//
//	//Load videos
//	load_video(path1, pFormatCtx1, &pCodecCtx1, &videostream1);
//	load_video(path2, pFormatCtx2, &pCodecCtx2, &videostream2);
////	load_video(path3, pFormatCtx3, &pCodecCtx3, &videostream3);
////	load_video(path4, pFormatCtx4, &pCodecCtx4, &videostream4);
////	load_video(path5, pFormatCtx5, &pCodecCtx5, &videostream5);
////	load_video(path6, pFormatCtx6, &pCodecCtx6, &videostream6);
////	load_video(path7, pFormatCtx7, &pCodecCtx7, &videostream7);
////	load_video(path8, pFormatCtx8, &pCodecCtx8, &videostream8);
//
//	sd1 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	sd2 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	//	sd3 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	//	sd4 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	//	sd5 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	//	sd6 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	//	sd7 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//	//	sd8 = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//
//	sd1 = (struct state_decompress *)init_decoder(&pCodecCtx1);
//	sd2 = (struct state_decompress *)init_decoder(&pCodecCtx2);
//	//	init_decoder(&pCodecCtx3, sd3);
//	//	init_decoder(&pCodecCtx4, sd4);
//	//	init_decoder(&pCodecCtx5, sd5);
//	//	init_decoder(&pCodecCtx6, sd6);
//	//	init_decoder(&pCodecCtx7, sd7);
//	//	init_decoder(&pCodecCtx8, sd8);
//
//	b1=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx1.width, pCodecCtx1.height)*sizeof(uint8_t));
//	b2=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx2.width, pCodecCtx2.height)*sizeof(uint8_t));
////	b3=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx3.width, pCodecCtx3.height)*sizeof(uint8_t));
////	b4=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx4.width, pCodecCtx4.height)*sizeof(uint8_t));
////	b5=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx5.width, pCodecCtx5.height)*sizeof(uint8_t));
////	b6=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx6.width, pCodecCtx6.height)*sizeof(uint8_t));
////	b7=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx7.width, pCodecCtx7.height)*sizeof(uint8_t));
////	b8=(uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_UYVY422, pCodecCtx8.width, pCodecCtx8.height)*sizeof(uint8_t));
//
//	//Init layout
//	Layout layout;
//	layout.init(1920, 1080, PIX_FMT_RGB24, 8);
//	printf ("Initializing layout 1920x1080 and 8 streams...\n");
//
//	begin = clock();
//
//	//Introduce streams
//	int id1 = layout.introduce_stream(pCodecCtx1.width, pCodecCtx1.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//	int id2 = layout.introduce_stream(pCodecCtx2.width, pCodecCtx2.height, PIX_FMT_UYVY422, 720, 480, 720, 480, PIX_FMT_RGB24, 0);
////	int id3 = layout.introduce_stream(pCodecCtx3.width, pCodecCtx2.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
////	int id4 = layout.introduce_stream(pCodecCtx4.width, pCodecCtx4.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
////	int id5 = layout.introduce_stream(pCodecCtx5.width, pCodecCtx5.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
////	int id6 = layout.introduce_stream(pCodecCtx6.width, pCodecCtx6.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
////	int id7 = layout.introduce_stream(pCodecCtx7.width, pCodecCtx7.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
////	int id8 = layout.introduce_stream(pCodecCtx8.width, pCodecCtx8.height, PIX_FMT_UYVY422, 720, 480, 0, 0, PIX_FMT_RGB24, 0);
//
//    // Main loop: for every stream, read frame, introduce it to the mixer, merge frames and save the layout a to a .ppm file
//    while(1){
//
//    	begin_reading = clock();
//
//    	ret1 = read_frame(pFormatCtx1, videostream1, &pCodecCtx1, b1, sd1, frame_cont);
//    	ret2 = read_frame(pFormatCtx2, videostream2, &pCodecCtx2, b2, sd2, frame_cont);
////    	ret3 = read_frame(pFormatCtx3, videostream3, &pCodecCtx3, b3, sd3, frame_cont);
////    	ret4 = read_frame(pFormatCtx4, videostream4, &pCodecCtx4, b4, sd4, frame_cont);
////    	ret5 = read_frame(pFormatCtx5, videostream5, &pCodecCtx5, b5, sd5, frame_cont);
////    	ret6 = read_frame(pFormatCtx6, videostream6, &pCodecCtx6, b6, sd6, frame_cont);
////    	ret7 = read_frame(pFormatCtx7, videostream7, &pCodecCtx7, b7, sd7, frame_cont);
////    	ret8 = read_frame(pFormatCtx8, videostream8, &pCodecCtx8, b8, sd8, frame_cont);
//
//    	end_reading = clock();
//
//    	if (ret1<0 || ret2<0 || ret3<0 || ret4<0 || ret5<0 || ret6<0 || ret7<0 || ret8<0){
//    		break;
//    	}
//
//    	layout.introduce_frame(id1, b1);
//    	layout.introduce_frame(id2, b2);
////    	layout.introduce_frame(id3, b3);
////    	layout.introduce_frame(id4, b4);
////    	layout.introduce_frame(id5, b5);
////    	layout.introduce_frame(id6, b6);
////    	layout.introduce_frame(id7, b7);
////    	layout.introduce_frame(id8, b8);
//
//    	layout.merge_frames();
//
//    	SaveFrame(layout.get_lay_frame(), layout.get_w(), layout.get_h(), frame_cont);
//
//    	reading_secs += (((float)end_reading)-((float)begin_reading))/CLOCKS_PER_SEC;
//
//    	printf("Frame: %d\n", frame_cont++);
//
//    	if(frame_cont>400){
//    		break;
//    	}
//   }
//
//    end = clock();
//
//    elapsed_secs = ((((float)end)-((float)begin))/CLOCKS_PER_SEC) - reading_secs;
//
//    printf("Elapsed time: %f",elapsed_secs);
//
//    return 0;
//}
//
//void*  init_decoder(AVCodecContext *pCodecCtx){
//	struct state_decompress *sd;
//
//	sd = (struct state_decompress *) calloc(2, sizeof(struct state_decompress *));
//
//	printf("Trying to initialize decompressor\n");
//
//	initialize_video_decompress();
//	printf("Decompressor initialized ;^)\n");
//
//	printf("Trying to initialize decoder\n");
//	if (decompress_is_available(LIBAVCODEC_MAGIC)){
//		sd = decompress_init(LIBAVCODEC_MAGIC);
//	}
//
//	printf("Decoder initialized ;^)\n");
//
//	struct video_desc des;
//
//	des.width = pCodecCtx->width;
//	des.height = pCodecCtx->height;
//	des.color_spec  = H264;
//	des.tile_count = 0;
//	des.interlacing = PROGRESSIVE;
//
//	decompress_reconfigure(sd, des, 0, 0, 0, vc_get_linesize(pCodecCtx->width, UYVY), UYVY);
//
//	set_codec(sd, pCodecCtx);
//
//	return sd;
//}
//
//int load_video(const char* path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx, int *videostream){
//
//	AVDictionary *optionsDict = NULL;
//	AVCodec *pCodec = NULL;
//	AVCodecContext *aux_codec_ctx = NULL;
//	int i;
//
//
//	// Open video file
//	if(avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0)
//		return -1; // Couldn't open file
//
//	// Retrieve stream information
//	if(avformat_find_stream_info(pFormatCtx, NULL)<0)
//		return -1; // Couldn't find stream information
//
//	// Find the first video stream
//	*videostream=-1;
//	for(i=0; i<pFormatCtx->nb_streams; i++){
//		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
//			*videostream=i;
//			break;
//		}
//	}
//	if(*videostream==-1)
//		return -1; // Didn't find a video stream
//
//	// Get a pointer to the codec context for the video stream
//	aux_codec_ctx=pFormatCtx->streams[*videostream]->codec;
//
//	// Find the decoder for the video stream
//	pCodec=avcodec_find_decoder(aux_codec_ctx->codec_id);
//	if(pCodec==NULL) {
//		fprintf(stderr, "Unsupported codec!\n");
//		return -1; // Codec not found
//	}
//
//	// Open codec
//	if(avcodec_open2(aux_codec_ctx, pCodec, &optionsDict)<0)
//		return -1; // Could not open codec
//
//	*pCodecCtx = *aux_codec_ctx;
//
//	return 0;
//}
//
//void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
//  FILE *pFile;
//  char szFilename[64];
//  int  y;
//
//  // Open file
//  sprintf(szFilename, "/home/palau/TFG/layout_prints/decoded1/layout%d.ppm", iFrame);
//  pFile=fopen(szFilename, "wb");
//  if(pFile==NULL)
//    return;
//
//  // Write header
//  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
//
//  // Write pixel data
//  for(y=0; y<height; y++)
//    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
//
//  // Close file
//  fclose(pFile);
//}
//
//int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff, struct state_decompress *sd, int frame_cont){
//	AVPacket packet;
//	int ret=FALSE;
//
//	while(ret==FALSE){
//		if(av_read_frame(pFormatCtx, &packet)<0){
//			return -1;
//		}
//
//		if(packet.stream_index==videostream) {
//			// Decode video frame
//			if(decompress_frame(sd, buff, packet.data, packet.size, frame_cont)==1){
//				ret=TRUE;
//			}
//			// Free the packet that was allocated by av_read_frame
//			av_free_packet(&packet);
//		}
//	}
//
//	return 0;
//}


