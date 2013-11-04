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
#include <map>
#include <string>
#include "layout.h"
extern "C"{
	#include <io_mngr/receiver.h>
	#include <io_mngr/transmitter.h>
}


using namespace std;

class mixer {
	public:
		struct Dst{
			char *ip;
			int port;
		};

		static mixer* get_instance();
		void init(uint32_t layout_width, uint32_t layout_height, uint32_t max_streams, uint32_t in_port, uint32_t out_port);
		void exec();
		void stop();
		int add_source();
		int remove_source(uint32_t id);
		int add_destination(char *ip, uint32_t port);
		int remove_destination(uint32_t id);
		int modify_stream (uint32_t id, uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t layer, bool keep_aspect_ratio);
		int resize_output (uint32_t width, uint32_t height, bool resize_streams);
		void change_max_framerate(uint32_t frame_rate);
		void show_stream_info();
		void get_stream_info(std::map<string,uint32_t> &str_map, uint32_t id);
		int get_destination(int id, std::string &ip, int *port);
		std::vector<uint32_t> get_streams_id();
		map<uint32_t, Dst> get_destinations();
		int change_stream_state(uint32_t id, stream_state_t state);
		int get_layout_size(int *width, int *height);
		void* run(void);
		static void* execute_run(void *context);
		uint8_t get_state();
		void set_state(uint8_t s);

	private:
		bool have_new_frame;
		pthread_t thread;
		receiver_t *receiver;
		transmitter_t *transmitter;
		stream_list *src_str_list;
		stream_list *dst_str_list;
		Layout *layout;
		bool should_stop;
		uint32_t dst_counter;
		int max_frame_rate;
		uint32_t _in_port;
		uint32_t _out_port;
		uint8_t state;

		map<uint32_t, Dst> destinations;

		static mixer* mixer_instance;
		mixer();


};

#endif /* MIXER_H_ */
