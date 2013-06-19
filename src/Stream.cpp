
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
}
#include <stream.h>
#include <iostream>

using namespace std;

void* Stream::resize(void){
	struct SwsContext *ctx;

	while (1) {

	//Check if the original frame is ready
	pthread_mutex_lock(&orig_frame_ready_mutex);
	while (!orig_frame_ready) {
		pthread_cond_wait(&orig_frame_ready_cond, &orig_frame_ready_mutex);
	}
	orig_frame_ready = false;
	pthread_mutex_unlock(&orig_frame_ready_mutex);

	#ifdef ENABLE_DEBUG
		cout << "Stream " << id << " resizing thread has been waken up" << endl;
	#endif

	pthread_mutex_lock(&resize_mutex);
	//Check if frame needs resizing
	if (orig_w == curr_w && orig_h == curr_h && orig_cp == curr_cp){
		if (curr_frame == NULL || curr_frame == orig_frame){
			curr_frame = orig_frame;  //Pointer curr_frame now points to orig_frame
		} else {
			av_freep(curr_frame);
			curr_frame = orig_frame;
		}

	}else{
		if (curr_frame == orig_frame){
			curr_frame = avcodec_alloc_frame();
			int num_bytes = avpicture_get_size(curr_cp, curr_w, curr_h);
			uint8_t *buffer = (uint8_t *) av_malloc(num_bytes * sizeof(uint8_t));
			avpicture_fill((AVPicture *) curr_frame, buffer, curr_cp, curr_w, curr_h);
		}

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
	}

	pthread_mutex_unlock(&resize_mutex);

	}
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

pthread_t* Stream::get_thread(){
	return thread;
}

void Stream::set_thread(pthread_t *thr){
	thread = thr;
}

bool Stream::is_orig_frame_ready(){
	return orig_frame_ready;
}

void Stream::set_orig_frame_ready(bool ready){
	orig_frame_ready = ready;
}

bool Stream::is_curr_frame_ready(){
	return curr_frame_ready;
}

void Stream::set_curr_frame_ready(bool ready){
	curr_frame_ready = ready;
}

pthread_mutex_t* Stream::get_orig_frame_ready_mutex(){
	return &orig_frame_ready_mutex;
}

pthread_cond_t*  Stream::get_orig_frame_ready_cond(){
	return &orig_frame_ready_cond;
}

pthread_mutex_t* Stream::get_resize_mutex(){
	return &resize_mutex;
}

pthread_mutex_t* Stream::get_needs_displaying_mutex(){
	return &needs_displaying_mutex;
}












