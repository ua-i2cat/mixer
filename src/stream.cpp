
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
}
#include <pthread.h>
#include "include/stream.h"
#include <iostream>
#include <stdio.h>

using namespace std;

Stream::Stream(int identifier, pthread_t thr){
	id = identifier;
	orig_w = 0;
	orig_h = 0;
	curr_w = 0;
	curr_h = 0;
	x_pos = 0;
	y_pos = 0;
	layer = 0;
	orig_cp = PIX_FMT_NONE;
	curr_cp = PIX_FMT_RGB24;
	needs_displaying = false;
	orig_frame_ready = false;
	current_frame_ready = false;
	first_frame = false;
	thread = thr;
	orig_frame = avcodec_alloc_frame();
	curr_frame = avcodec_alloc_frame();
	dummy_frame = avcodec_alloc_frame();
	buffer = NULL;
	dummy_buffer = NULL;
	in_buffer = NULL;
	orig_frame_ready_mutex = PTHREAD_MUTEX_INITIALIZER;
	resize_mutex = PTHREAD_MUTEX_INITIALIZER;
	orig_frame_ready_cond = PTHREAD_COND_INITIALIZER;
	first_frame_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_rwlock_init(&needs_displaying_rwlock, NULL);
	pthread_rwlock_init(&current_frame_ready_rwlock, NULL);

}


void* Stream::resize(void){
	struct SwsContext *ctx;

	while (1) {

		//Check if the original frame is ready
		pthread_mutex_lock(&orig_frame_ready_mutex);
		while (!orig_frame_ready) {
#ifdef ENABLE_DEBUG
			printf("Stream %d resizing thread is in waiting loop\n", id);
#endif
		    pthread_cond_wait(&orig_frame_ready_cond, &orig_frame_ready_mutex);
		}
		orig_frame_ready = false;
		pthread_mutex_unlock(&orig_frame_ready_mutex);
		
#ifdef ENABLE_DEBUG
		cout << "Stream " << id << " resizing thread has been waken up" << endl;
#endif

		pthread_mutex_lock(&resize_mutex);
		//Check if frame needs resizing
//		if (orig_w == curr_w && orig_h == curr_h && orig_cp == curr_cp){
//				curr_frame = orig_frame;  //Pointer curr_frame now points to orig_frame
//
//		}else{
//			if (curr_frame == orig_frame){
//				curr_frame = avcodec_alloc_frame();
//				buffsize = avpicture_get_size(curr_cp, curr_w, curr_h) * 10 * sizeof(uint8_t);
//				free(buffer);
//				buffer = (uint8_t*)malloc(buffsize);
//				avpicture_fill((AVPicture *)curr_frame, buffer, curr_cp, curr_w, curr_h);
//			}
            
			//Prepare context
			ctx = sws_getContext(
				orig_w,
				orig_h,
				orig_cp,
				curr_w,
				curr_h,
				curr_cp,
				SWS_BILINEAR,
				NULL,
				NULL,
				NULL
			);
            
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

//		}

        
		pthread_mutex_unlock(&resize_mutex);
			
		pthread_rwlock_wrlock(&current_frame_ready_rwlock);
		current_frame_ready = true;
		pthread_rwlock_unlock(&current_frame_ready_rwlock);

	}
}

void Stream::set_stream_to_default(){

	avcodec_get_frame_defaults(orig_frame);
	avcodec_get_frame_defaults(curr_frame);
	avcodec_get_frame_defaults(dummy_frame);

	if (buffer != NULL){
		free(buffer);
	}
	if (dummy_buffer != NULL){
		free(dummy_buffer);
	}
	if (in_buffer != NULL){
		free(in_buffer);
	}

	orig_w = 0;
	orig_h = 0;
	curr_w = 0;
	curr_h = 0;
	x_pos = 0;
	y_pos = 0;
	layer = 0;
	orig_cp = PIX_FMT_NONE;
	curr_cp = PIX_FMT_RGB24;
	needs_displaying = false;
	orig_frame_ready = false;
	current_frame_ready = false;
	first_frame = false;
}

void* Stream::execute_resize(void *context){
	return ((Stream *)context)->resize();
}

int Stream::get_id(){
	return id;
}

void Stream::set_id(int set_id){
	id = set_id;
}

int Stream::get_orig_w(){
	return orig_w;
}

void Stream::set_orig_w(int set_orig_w){
	orig_w = set_orig_w;
}

int Stream::get_orig_h(){
	return orig_h;
}

void Stream::set_orig_h(int set_orig_h){
	orig_h = set_orig_h;
}

int Stream::get_curr_w(){
	return curr_w;
}

void Stream::set_curr_w(int set_curr_w){
	curr_w = set_curr_w;
}

int Stream::get_curr_h(){
	return curr_h;
}
void Stream::set_curr_h(int set_curr_h){
	curr_h = set_curr_h;
}

int Stream::get_x_pos(){
	return x_pos;
}

void Stream::set_x_pos(int set_x_pos){
	x_pos = set_x_pos;
}

int Stream::get_y_pos(){
	return y_pos;
}

void Stream::set_y_pos(int set_y_pos){
	y_pos = set_y_pos;
}

int Stream::get_layer(){
	return layer;
}

void Stream::set_layer(int set_layer){
	layer = set_layer;
}

enum PixelFormat Stream::get_orig_cp(){
	return orig_cp;
}

void Stream::set_orig_cp(enum PixelFormat set_orig_cp){
	orig_cp = set_orig_cp;
}

enum PixelFormat Stream::get_curr_cp(){
	return curr_cp;
}

void Stream::set_curr_cp(enum PixelFormat set_curr_cp){
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

bool Stream::get_needs_displaying(){
	return needs_displaying;
}

void Stream::set_needs_displaying(bool set_needs_displaying){
	needs_displaying = set_needs_displaying;
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

bool Stream::is_current_frame_ready(){
	return current_frame_ready;
}

void Stream::set_current_frame_ready(bool ready){
	current_frame_ready = ready;
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

pthread_mutex_t* Stream::get_resize_mutex(){
	return &resize_mutex;
}

void Stream::set_resize_mutex(pthread_mutex_t mutex){
	resize_mutex = mutex;
}

pthread_rwlock_t* Stream::get_needs_displaying_rwlock(){
	return &needs_displaying_rwlock;
}

void Stream::set_needs_displaying_rwlock(pthread_rwlock_t lock){
	needs_displaying_rwlock = lock;
}

unsigned int* Stream::get_buffsize(){
	return &buffsize;
}

void Stream::set_buffsize(unsigned int bsize){
	buffsize = bsize;
}

unsigned int Stream::get_in_buffsize(){
	return in_buffsize;
}
void Stream::set_in_buffsize (unsigned int bsize){
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

pthread_rwlock_t* Stream::get_current_frame_ready_rwlock(){
	return &current_frame_ready_rwlock;
}

void Stream::set_current_frame_ready_rwlock(pthread_rwlock_t lock){
	current_frame_ready_rwlock = lock;
}

pthread_mutex_t* Stream::get_first_frame_mutex(){
	return &first_frame_mutex;
}

void Stream::set_next_frame_mutex(pthread_mutex_t mutex){
	first_frame_mutex = mutex;
}

bool Stream::has_first_frame(){
	return first_frame;
}

void Stream::set_first_frame(bool first){
	first_frame = first;
}















