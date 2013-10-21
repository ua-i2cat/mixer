
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
}
#include <pthread.h>
#include "stream.h"
#include <iostream>
#include <stdio.h>

using namespace std;

Stream::Stream(uint32_t identifier, pthread_rwlock_t* lock, uint32_t orig_width, uint32_t orig_height, 
				enum AVPixelFormat orig_colorspace, uint32_t new_width, uint32_t new_height, 
				enum AVPixelFormat new_colorspace, uint32_t x, uint32_t y, uint32_t print_layer){
	
	//Set stream info
	id = identifier;
	orig_w = orig_width;
	orig_h = orig_height;
	curr_w = new_width;
	curr_h = new_height;
	x_pos = x;
	y_pos = y;
	layer = print_layer;
	active = 0;
	orig_cp = orig_colorspace;
	curr_cp = new_colorspace;
	
	//Generate AVFrame structures
	orig_frame = avcodec_alloc_frame();
	curr_frame = avcodec_alloc_frame();
	dummy_frame = avcodec_alloc_frame();
	ctx = sws_alloc_context();
	in_buffer = NULL;
	buffsize = avpicture_get_size(curr_cp, curr_w, curr_h) * sizeof(uint8_t);
	buffer = (uint8_t*)malloc(buffsize);
	dummy_buffer = (uint8_t*)malloc(buffsize);
	avpicture_fill((AVPicture *)curr_frame, buffer, curr_cp, curr_w, curr_h);
	avpicture_fill((AVPicture *)dummy_frame, dummy_buffer, curr_cp, curr_w, curr_h);
	in_buffsize = avpicture_get_size(orig_cp, orig_w, orig_h) * sizeof(uint8_t);
	in_buffer = (uint8_t*)malloc(in_buffsize);
	avpicture_fill((AVPicture *)orig_frame, in_buffer, orig_cp, orig_w, orig_h);
	ctx = sws_getContext(orig_w, orig_h, orig_cp, curr_w, curr_h, curr_cp, SWS_BILINEAR, NULL, NULL, NULL);

	//Init concurrence mutex and flags
	orig_frame_ready = false;
	pthread_mutex_init(&orig_frame_ready_mutex, NULL);
	pthread_mutex_init(&in_buffer_mutex, NULL);
	pthread_cond_init(&orig_frame_ready_cond, NULL);
	stream_resize_rwlock_ref = lock;
	
	//Create resizing thread
	pthread_create(&thread, NULL, Stream::execute_resize, this);

}

Stream::~Stream(){
	if (orig_frame != NULL)
		avcodec_free_frame(&orig_frame);
	if (curr_frame != NULL)
		avcodec_free_frame(&curr_frame);
	if (dummy_frame != NULL)
		avcodec_free_frame(&dummy_frame);
	if (buffer != NULL)
		free(buffer);
	if (dummy_buffer != NULL)
		free(dummy_buffer);
	if (in_buffer != NULL)
		free(in_buffer);
	if (ctx != NULL)
		sws_freeContext(ctx);
}


void* Stream::resize(void){

	while (1) {

		//Check if the original frame is ready
		pthread_mutex_lock(&orig_frame_ready_mutex);
		while (!orig_frame_ready) {
		    pthread_cond_wait(&orig_frame_ready_cond, &orig_frame_ready_mutex);
		}

		orig_frame_ready = false;
		pthread_mutex_unlock(&orig_frame_ready_mutex);

		pthread_rwlock_rdlock(stream_resize_rwlock_ref);
            
			//Scale
			sws_scale(
				ctx,
				(uint8_t const * const *)orig_frame->data,
				orig_frame->linesize,
				0,
				orig_h,
				curr_frame->data,
				curr_frame->linesize
			);

		pthread_rwlock_unlock(stream_resize_rwlock_ref);

	}
}

void Stream::set_stream_to_default(){

	pthread_mutex_lock(&in_buffer_mutex);

	avcodec_get_frame_defaults(orig_frame);
	avcodec_get_frame_defaults(curr_frame);
	avcodec_get_frame_defaults(dummy_frame);

	if (buffer != NULL){
		free(buffer);
		buffer = NULL;
	}
	if (dummy_buffer != NULL){
		free(dummy_buffer);
		dummy_buffer = NULL;
	}
	if (in_buffer != NULL){
		free(in_buffer);
		in_buffer = NULL;
	}

	pthread_mutex_unlock(&in_buffer_mutex);

	orig_w = 0;
	orig_h = 0;
	curr_w = 0;
	curr_h = 0;
	x_pos = 0;
	y_pos = 0;
	layer = 0;
	orig_cp = PIX_FMT_NONE;
	curr_cp = PIX_FMT_RGB24;
	orig_frame_ready = false;
	sws_freeContext(ctx);
	ctx = NULL;
}

void* Stream::execute_resize(void *context){
	return ((Stream *)context)->resize();
}

uint32_t Stream::get_id(){
	return id;
}

void Stream::set_id(uint32_t set_id){
	id = set_id;
}

uint32_t Stream::get_orig_w(){
	return orig_w;
}

void Stream::set_orig_w(uint32_t set_orig_w){
	orig_w = set_orig_w;
}

uint32_t Stream::get_orig_h(){
	return orig_h;
}

void Stream::set_orig_h(uint32_t set_orig_h){
	orig_h = set_orig_h;
}

uint32_t Stream::get_curr_w(){
	return curr_w;
}

void Stream::set_curr_w(uint32_t set_curr_w){
	curr_w = set_curr_w;
}

uint32_t Stream::get_curr_h(){
	return curr_h;
}
void Stream::set_curr_h(uint32_t set_curr_h){
	curr_h = set_curr_h;
}

uint32_t Stream::get_x_pos(){
	return x_pos;
}

void Stream::set_x_pos(uint32_t set_x_pos){
	x_pos = set_x_pos;
}

uint32_t Stream::get_y_pos(){
	return y_pos;
}

void Stream::set_y_pos(uint32_t set_y_pos){
	y_pos = set_y_pos;
}

uint32_t Stream::get_layer(){
	return layer;
}

void Stream::set_layer(uint32_t set_layer){
	layer = set_layer;
}

enum AVPixelFormat Stream::get_orig_cp(){
	return orig_cp;
}

void Stream::set_orig_cp(enum AVPixelFormat set_orig_cp){
	orig_cp = set_orig_cp;
}

enum AVPixelFormat Stream::get_curr_cp(){
	return curr_cp;
}

void Stream::set_curr_cp(enum AVPixelFormat set_curr_cp){
	curr_cp = set_curr_cp;
}

AVFrame* Stream::get_orig_frame(){
	return orig_frame;
}

void Stream::set_orig_frame(AVFrame *set_orig_frame){
	orig_frame = set_orig_frame;
}

AVFrame* Stream::get_current_frame(){
	return curr_frame;
}

void Stream::set_current_frame(AVFrame *set_curr_frame){
	curr_frame = set_curr_frame;
}

pthread_t Stream::get_thread(){
	return thread;
}

void Stream::set_thread(pthread_t thr){
	thread = thr;
}

bool Stream::is_orig_frame_ready(){
	return orig_frame_ready;
}

void Stream::set_orig_frame_ready(bool ready){
	orig_frame_ready = ready;
}

pthread_mutex_t* Stream::get_orig_frame_ready_mutex(){
	return &orig_frame_ready_mutex;
}

void Stream::set_orig_frame_ready_mutex(pthread_mutex_t mutex){
	orig_frame_ready_mutex = mutex;
}

pthread_cond_t*  Stream::get_orig_frame_ready_cond(){
	return &orig_frame_ready_cond;
}

void  Stream::set_orig_frame_ready_cond(pthread_cond_t cond){
	orig_frame_ready_cond = cond;
}

pthread_mutex_t* Stream::get_in_buffer_mutex(){
	return &in_buffer_mutex;
}

void Stream::set_in_buffer_mutex(pthread_mutex_t mutex){
	in_buffer_mutex = mutex;
}

uint32_t Stream::get_buffsize(){
	return buffsize;
}

void Stream::set_buffsize(uint32_t bsize){
	buffsize = bsize;
}

uint32_t Stream::get_in_buffsize(){
	return in_buffsize;
}
void Stream::set_in_buffsize (uint32_t bsize){
	in_buffsize = bsize;
}

uint8_t* Stream::get_buffer(){
	return buffer;
}

void Stream::set_buffer(uint8_t *buff){
	buffer = buff;
}

AVFrame* Stream::get_dummy_frame(){
	return dummy_frame;
}

void Stream::set_dummy_frame(AVFrame *set_dummy_frame){
	dummy_frame = set_dummy_frame;
}

uint8_t* Stream::get_dummy_buffer(){
	return dummy_buffer;
}

void  Stream::set_dummy_buffer(uint8_t* buff){
	dummy_buffer = buff;
}

uint8_t* Stream::get_in_buffer(){
	return in_buffer;
}

void Stream::set_in_buffer(uint8_t* buff){
	in_buffer = buff;
}

struct SwsContext* Stream::get_ctx(){
	return ctx;
}

void Stream::set_ctx(struct SwsContext *context){
	ctx = context;
}

uint8_t Stream::get_active(){
	return active;
}

void Stream::set_active(uint8_t active_flag){
	active = active_flag;
}














