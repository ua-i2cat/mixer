/*
 * mixer.h
 *
 *  Created on: Jul 17, 2013
 *      Author: palau
 */

#ifndef MIXER_H_
#define MIXER_H_

#include <pthread.h>
#include <stdint.h>
#include "layout.h"
extern "C"{
	#include "reciever.h"
}


using namespace std;

class mixer {

		pthread_t thread;
		reciever_t *reciever;
		participant_list *src_p_list;
		participant_list *dst_p_list;
		Layout layout;
		bool should_stop;
		uint32_t dst_counter;
		int max_frame_rate;
		uint32_t _in_port;
		uint32_t _out_port;

		static mixer* mixer_instance;
		mixer();

	public:
		static mixer* get_instance();
		void init(int layout_width, int layout_height, int max_streams, uint32_t in_port, uint32_t out_port);
		void exec();
		void stop();
		int add_source(uint32_t width, uint32_t height, codec_t codec);
		int remove_source(uint32_t id);
		int add_destination(codec_t codec, char *ip, uint32_t port);
		int remove_destination(uint32_t id);
		int modify_stream (int id, int width, int height, int x, int y, int layer, bool keep_aspect_ratio);
		int resize_output (int width, int height, bool resize_streams);
		void change_max_framerate(int frame_rate);
		void show_stream_info();
		void* run(void);
		static void* execute_run(void *context);


};

#endif /* MIXER_H_ */
