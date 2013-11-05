#include "layout.h"

#include <stdio.h>
#include <unistd.h>

int load_video(const char *path, AVFormatContext *pFormatCtx, AVCodecContext *pCodecCtx);
int read_frame(AVFormatContext *pFormatCtx, int videostream, AVCodecContext *pCodecCtx, uint8_t *buff);

int main(int argc, char *argv[]){

	char *OUTPUT_PATH = "rx_frame.yuv";
	char *outpath1 = "out1.rgb";
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

	uint32_t layout_width = 1920;
	uint32_t layout_height = 1080;
	uint32_t max_streams = 8;

	const char *path1 = "/home/palau/Videos/sintel-1024-surround-2.mp4";
	const char *path2 = "/home/palau/Videos/big_buck_bunny_480p_h264.mov";

	fctx1 = avformat_alloc_context();
	fctx2 = avformat_alloc_context();

	v1 = load_video(path1, fctx1, &cctx1);
	v2 = load_video(path2, fctx2, &cctx2);

	//printf("Pixel Format = %d, Width = %d, Height = %d\n", cctx1.pix_fmt, cctx1.width, cctx1.height);

	bsize1 = avpicture_get_size(cctx1.pix_fmt, cctx1.width, cctx1.height)*sizeof(uint8_t);
	bsize2 = avpicture_get_size(cctx2.pix_fmt, cctx2.width, cctx2.height)*sizeof(uint8_t);

	b1=(uint8_t *)av_malloc(bsize1);
	b2=(uint8_t *)av_malloc(bsize2);
	
	Layout *layout = new Layout(layout_width, layout_height, PIX_FMT_RGB24, max_streams); 

	layout->introduce_stream(1, cctx1.width, cctx1.height, cctx1.pix_fmt, 500, 500, PIX_FMT_RGB24, 0, 0, 0);
	layout->introduce_stream(2, cctx2.width, cctx2.height, cctx2.pix_fmt, 854, 480, PIX_FMT_RGB24, 300, 300, 0);

	printf("Introduced streams\n");

	while(1){

    	read_frame(fctx1, v1, &cctx1, b1);
    	read_frame(fctx2, v2, &cctx2, b2);

    	layout->introduce_frame(1, b1, bsize1);
	  	layout->introduce_frame(2, b2, bsize2);

    	layout->merge_frames();

		if (F_video_rx == NULL) {
			printf("recording rx frame...\n");
		 	F_video_rx = fopen(OUTPUT_PATH, "wb");
		}

		if (cont == 1200){
		 	layout->set_active(1, 0);
		 	printf("Modified stream\n");
		}

		if (cont == 1280){
			layout->set_active(1, 1);
		 	printf("Modified stream\n");
		}

		if (cont > 1000){
		 	fwrite(layout->get_layout_bytestream(), layout->get_buffsize(), 1, F_video_rx);
		}

		cont++;

		if (cont > 100){
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

	printf("Pixel Format = %d, Width = %d, Height = %d\n", ctx->pix_fmt, ctx->width, ctx->height);

  return videoStream;
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
		}
	}
	
	av_free_packet(&packet);
	return ret;
}
