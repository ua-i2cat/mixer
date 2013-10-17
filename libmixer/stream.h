/*
 * Stream.h
 *
 *  Created on: Jun 13, 2013
 *      Author: palau
 */

#ifndef STREAM_H_
#define STREAM_H_

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
}
#include <pthread.h>

class Stream {

	private:
		int id, orig_w, orig_h, curr_w, curr_h, x_pos, y_pos, layer;
		enum AVPixelFormat orig_cp, curr_cp;
		AVFrame *orig_frame, *curr_frame, *dummy_frame;
		bool needs_displaying, orig_frame_ready;
		pthread_t* thread;
		pthread_mutex_t in_buffer_mutex, orig_frame_ready_mutex;
		pthread_rwlock_t needs_displaying_rwlock, *stream_resize_rwlock_ref;
		pthread_cond_t  orig_frame_ready_cond;
		unsigned int buffsize, in_buffsize;
		uint8_t *buffer, *dummy_buffer, *in_buffer;
		struct SwsContext *ctx;
		uint8_t active;


	public:
		Stream(int identifier, pthread_t *thr, pthread_rwlock_t* lock);
		~Stream();
		int get_id();
		void set_id(int set_id);
		int get_orig_w();
		void set_orig_w(int set_orig_w);
		int get_orig_h();
		void set_orig_h(int set_orig_h);
		int get_curr_w();
		void set_curr_w(int set_curr_w);
		int get_curr_h();
		void set_curr_h(int set_curr_h);
		int get_x_pos();
		void set_x_pos(int set_x_pos);
		int get_y_pos();
		void set_y_pos(int set_y_pos);
		int get_layer();
		void set_layer(int set_layer);
		unsigned int* get_buffsize();
		void set_buffsize(unsigned int bsize);
		unsigned int get_in_buffsize();
		void set_in_buffsize (unsigned int bsize);
		uint8_t* get_buffer();
		void set_buffer(uint8_t *buff);
		enum AVPixelFormat get_orig_cp();
		void set_orig_cp(enum AVPixelFormat set_orig_cp);
		enum AVPixelFormat get_curr_cp();
		void set_curr_cp(enum AVPixelFormat set_curr_cp);
		AVFrame* get_orig_frame();
		void set_orig_frame(AVFrame *set_orig_frame);
		AVFrame* get_current_frame();
		void set_current_frame(AVFrame *set_curr_frame);
		AVFrame* get_dummy_frame();
		void set_dummy_frame(AVFrame *set_dummy_frame);
		uint8_t* get_dummy_buffer();
		void  set_dummy_buffer(uint8_t* buff);
		uint8_t* get_in_buffer();
		void set_in_buffer(uint8_t* buff);
		bool get_needs_displaying();
		void set_needs_displaying(bool set_needs_displaying);
		pthread_t* get_thread();
		void set_thread(pthread_t *thr);
		bool is_orig_frame_ready();
		void set_orig_frame_ready(bool ready);
		pthread_mutex_t* get_orig_frame_ready_mutex();
		void set_orig_frame_ready_mutex(pthread_mutex_t mutex);
		pthread_cond_t*  get_orig_frame_ready_cond();
		void  set_orig_frame_ready_cond(pthread_cond_t cond);
		pthread_mutex_t* get_in_buffer_mutex();
		void set_in_buffer_mutex(pthread_mutex_t mutex);
		pthread_rwlock_t* get_needs_displaying_rwlock();
		void set_needs_displaying_rwlock(pthread_rwlock_t lock);
		void set_stream_to_default();
		struct SwsContext* get_ctx();
		void set_ctx(struct SwsContext *context);
		uint8_t get_active();
		void set_active(uint8_t active_flag);


		void *resize(void);
		static void *execute_resize(void *context);

};



#endif /* STREAM_H_ */
