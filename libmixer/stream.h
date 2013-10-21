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
		uint32_t id, orig_w, orig_h, curr_w, curr_h, x_pos, y_pos, layer;
		enum AVPixelFormat orig_cp, curr_cp;
		AVFrame *orig_frame, *curr_frame, *dummy_frame;
		bool orig_frame_ready;
		pthread_t thread;
		pthread_mutex_t in_buffer_mutex, orig_frame_ready_mutex;
		pthread_rwlock_t *stream_resize_rwlock_ref;
		pthread_cond_t  orig_frame_ready_cond;
		uint32_t buffsize, in_buffsize;
		uint8_t *buffer, *dummy_buffer, *in_buffer;
		struct SwsContext *ctx;
		uint8_t active;


	public:
		Stream(uint32_t identifier, pthread_rwlock_t* lock, uint32_t orig_width, uint32_t orig_height, 
				enum AVPixelFormat orig_colorspace, uint32_t new_width, uint32_t new_height, 
				enum AVPixelFormat new_colorspace, uint32_t x, uint32_t y, uint32_t print_layer);
		~Stream();
		uint32_t get_id();
		void set_id(uint32_t set_id);
		uint32_t get_orig_w();
		void set_orig_w(uint32_t set_orig_w);
		uint32_t get_orig_h();
		void set_orig_h(uint32_t set_orig_h);
		uint32_t get_curr_w();
		void set_curr_w(uint32_t set_curr_w);
		uint32_t get_curr_h();
		void set_curr_h(uint32_t set_curr_h);
		uint32_t get_x_pos();
		void set_x_pos(uint32_t set_x_pos);
		uint32_t get_y_pos();
		void set_y_pos(uint32_t set_y_pos);
		uint32_t get_layer();
		void set_layer(uint32_t set_layer);
		uint32_t get_buffsize();
		void set_buffsize(uint32_t bsize);
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
		pthread_t get_thread();
		void set_thread(pthread_t thr);
		bool is_orig_frame_ready();
		void set_orig_frame_ready(bool ready);
		pthread_mutex_t* get_orig_frame_ready_mutex();
		void set_orig_frame_ready_mutex(pthread_mutex_t mutex);
		pthread_cond_t*  get_orig_frame_ready_cond();
		void  set_orig_frame_ready_cond(pthread_cond_t cond);
		pthread_mutex_t* get_in_buffer_mutex();
		void set_in_buffer_mutex(pthread_mutex_t mutex);
		void set_stream_to_default();
		struct SwsContext* get_ctx();
		void set_ctx(struct SwsContext *context);
		uint8_t get_active();
		void set_active(uint8_t active_flag);

		void *resize(void);
		static void *execute_resize(void *context);

};



#endif /* STREAM_H_ */
