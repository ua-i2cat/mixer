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
	#include <ug-modules/src/io_mngr/receiver.h>
}


using namespace std;

class mixer {
	public:
		struct Dst{
			char *ip;
			int port;
		};

		static mixer* get_instance();
		void init(int layout_width, int layout_height, int max_streams, uint32_t in_port, uint32_t out_port);
		void exec();
		void stop();
		int add_source(int width, int height, int new_w, int new_h, int x, int y, int layer, codec_t codec);
		int remove_source(uint32_t id);
		int add_destination(codec_t codec, char *ip, uint32_t port);
		int remove_destination(uint32_t id);
		int modify_stream (int id, int width, int height, int x, int y, int layer, bool keep_aspect_ratio);
		int resize_output (int width, int height, bool resize_streams);
		void change_max_framerate(int frame_rate);
		void show_stream_info();
		void get_stream_info(std::map<string, int> &str_map, int id);
		int get_destination(int id, std::string &ip, int *port);
		std::vector<int> get_streams_id();
		map<uint32_t, Dst> get_destinations();
		int set_stream_active(int id, uint8_t active_flag);
		int get_layout_size(int *width, int *height);
		void* run(void);
		static void* execute_run(void *context);
		uint8_t get_state();
		void set_state(uint8_t s);

	private:
		pthread_t thread;
		receiver_t *receiver;
		participant_list *src_p_list;
		participant_list *dst_p_list;
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
