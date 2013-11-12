#include "layout.h"
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
}

#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

int load_video(const char *path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx);
int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff);

int main(int argc, char *argv[]){
	struct timeval start, finish;
	float diff = 0, avg_diff = 0, diff_count = 0;

	char *OUTPUT_PATH = "rx_frame.yuv";
	char *OUTPUT_PATH1 = "out1.rgb";
	char *outpath2 = "out2.rgb";

	FILE *F_video_rx = NULL;
	FILE *F_video_rx1 = NULL;
	FILE *F_video_rx2 = NULL;

	AVFormatContext *fctx1, *fctx2;
	AVCodecContext cctx1, cctx2;
	int v1,v2;
	uint8_t *b1=NULL, *b2=NULL;
	uint32_t bsize1, bsize2;
	int cont=0;

	uint32_t layout_width = 1280;
	uint32_t layout_height = 720;

	const char *path1 = "/home/palau/Videos/sintel-1024-surround-2.mp4";
	const char *path2 = "/home/palau/Videos/big_buck_bunny_480p_h264.mov";

	fctx1 = avformat_alloc_context();
	fctx2 = avformat_alloc_context();

	v1 = load_video(path1, fctx1, &cctx1);
	v2 = load_video(path2, fctx2, &cctx2);

	bsize1 = avpicture_get_size(PIX_FMT_RGB24, cctx1.width, cctx1.height)*sizeof(uint8_t);
	bsize2 = avpicture_get_size(PIX_FMT_RGB24, cctx2.width, cctx2.height)*sizeof(uint8_t);

	b1=(uint8_t *)av_malloc(bsize1);
	b2=(uint8_t *)av_malloc(bsize2);
	
	Layout *layout = new Layout(layout_width, layout_height); 

	layout->add_stream(1, cctx1.width, cctx1.height);
	layout->add_stream(2, cctx2.width, cctx2.height);

	printf("Introduced streams\n");

	while(1){

    	read_frame(fctx1, v1, &cctx1, b1);
    	read_frame(fctx2, v2, &cctx2, b2);

    	gettimeofday(&start, NULL);    

    	layout->introduce_frame_to_stream(1, b1, bsize1);
    	layout->introduce_frame_to_stream(2, b2, bsize2);

    	layout->compose_layout();

    	gettimeofday(&finish, NULL);
		
		if (cont > 1000){

			diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec); // In us
			avg_diff += diff;
			diff_count++;

			if (F_video_rx == NULL) {
				printf("recording rx frame...\n");
		 		F_video_rx = fopen(OUTPUT_PATH, "wb");
			}

		 	fwrite(layout->get_buffer(), layout->get_buffer_size(), 1, F_video_rx);
		}

		cont++;
		

		// if (cont == 1300){
		//  	layout->add_crop_to_stream(1, 300, 300, 100, 100, 10, 200, 200, 1080, 0);
		//  	printf("New src crop\n");
		// }

		// if (cont == 1100){
		// 	layout->add_crop_to_stream(2, 300, 300, 100, 100, 20, 300, 300, 980, 420);
		// 	printf("New src crop\n");
		// }

		// if (cont == 1500){
		// 	layout->disable_crop_from_stream(1, layout->get_stream_by_id(1)->get_crops().begin()->first);
		// 	printf("Crop disabled\n");
		// }

		// if (cont == 1700){
		// 	layout->enable_crop_from_stream(1, layout->get_stream_by_id(1)->get_crops().begin()->first);
		// 	printf("Crop enabled\n");
		// }

		if (cont > 2000){
			avg_diff = avg_diff/diff_count;
			printf("Average time %f (us)\n", avg_diff);
			printf("Frame recording finished");
		 	return 0;
		}
	}
}

int load_video(const char *path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx) {
  int             i, videoStream;
  AVCodec         *pCodec = NULL;
  AVDictionary    *optionsDict = NULL;
  AVCodecContext *ctx = NULL;
  
  // Register all formats and codecs
  av_register_all();
  
  // Open video file
  if(avformat_open_input(&pFormatCtx, path, NULL, NULL)!=0)
    return -1; // Couldn't open file
  
  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, path, 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  ctx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(ctx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }
  // Open codec
  if(avcodec_open2(ctx, pCodec, &optionsDict)<0)
    return -1; // Could not open codec

	*pCodecCtx = *ctx;

  return videoStream;
}

int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff){
	AVPacket packet;
	AVFrame* pFrame, *pFrameRGB;
	int frameFinished, ret, numBytes;
	uint8_t* buffer;
	struct SwsContext *sws_ctx = NULL;

  	pFrame=avcodec_alloc_frame();
  	pFrameRGB=avcodec_alloc_frame();
  
  // Determine required buffer size and allocate buffer
  	numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
                              pCodecCtx->height);
  	buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  	sws_ctx =
    	sws_getContext
    	(
        	pCodecCtx->width,
        	pCodecCtx->height,
        	pCodecCtx->pix_fmt,
        	pCodecCtx->width,
        	pCodecCtx->height,
        	PIX_FMT_RGB24,
        	SWS_BILINEAR,
        	NULL,
        	NULL,
        	NULL
    	);
  
  	avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
                 pCodecCtx->width, pCodecCtx->height);

	pFrame = avcodec_alloc_frame();
	ret = av_read_frame(pFormatCtx, &packet);

	if(packet.stream_index==videostream) {
		// Decode video frame
		avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
		// Did we get a video frame?
		if(frameFinished) {
			sws_scale
        	(
            	sws_ctx,
            	(uint8_t const * const *)pFrame->data,
            	pFrame->linesize,
            	0,
            	pCodecCtx->height,
            	pFrameRGB->data,
            	pFrameRGB->linesize
        	);
			
			avpicture_layout((AVPicture *)pFrameRGB, PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, buff, numBytes);

			// Free the packet that was allocated by av_read_frame
		}
	}
	
	av_free(buffer);
  	av_free(pFrameRGB);
  	av_free(pFrame);
	av_free_packet(&packet);
	sws_freeContext(sws_ctx);
	return ret;
}
