/*
 * mixer.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: palau
 */

#include <iostream>
#include <map>
#include <string>
#include "mixer.h"
#include <sys/time.h>
extern "C"{
	#include <io_mngr/transmitter.h>
}

using namespace std;
mixer* mixer::mixer_instance;

void* mixer::run(void) {
	int i;
	stream_data_t *stream;
	have_new_frame = false;
	should_stop = false;
	struct timeval start, finish;
	float diff = 0, min_diff = 0;

	min_diff = ((float)1/(float)max_frame_rate)*1000; // In ms

	while (!should_stop){
	
	    min_diff = ((float)1/(float)max_frame_rate)*1000;

		if (diff < min_diff){
			usleep((min_diff - diff)*1000); 
		}
		gettimeofday(&start, NULL);

		pthread_rwlock_rdlock(&input_str_list->lock);

		stream = input_str_list->first;

		for (i=0; i<input_str_list->count; i++){
			pthread_rwlock_rdlock(&stream->lock);

			if(!check_if_layout_stream(stream->id)){
				pthread_rwlock_rdlock(stream->video.lock);
				introduce_stream(stream->id, stream->video.width, stream->video.height, PIX_FMT_RGB24, 
					stream->video.width, stream->video.height, PIX_FMT_RGB24, 0, 0, 0);
				pthread_rwlock_unlock(stream->video.lock);
			}

			pthread_mutex_lock(&stream->video.new_decoded_frame_lock)
			if (stream->video.new_decoded_frame){
				pthread_rwlock_rdlock(stream->video.lock);
				layout->introduce_frame(stream->id, (uint8_t*)stream->video.decoded_frame, stream->video.decoded_frame_len);
				pthread_rwlock_unlock(stream->video.lock);
				have_new_frame = true;
				stream->video.new_decoded_frame = FALSE;
			}
			pthread_mutex_unlock(&stream->video.new_decoded_frame_lock);

			pthread_mutex_unlock(&stream->lock);

			stream = stream->next;
		}

		pthread_rwlock_unlock(&input_str_list->lock);

		if (have_new_frame){
			layout->merge_frames();		
			pthread_rwlock_rdlock(&dst_str_list->lock);

			stream = dst_str_list->first;

			for (i=0; i<dst_str_list->count; i++){
				pthread_rwlock_rdlock(&stream->lock);

				pthread_rwlock_wrlock(&stream->video.lock);
				memcpy((uint8_t*)stream->video.decoded_frame, (uint8_t*)layout->get_layout_bytestream(), layout->get_buffsize());
				stream->video.decoded_frame_len = layout->get_buffsize();
				pthread_rwlock_unlock(&stream->video.lock);

				pthread_mutex_lock(&stream->video.new_decoded_frame_lock);
				stream->video.new_decoded_frame = TRUE;
				pthread_mutex_unlock(&stream->video.new_decoded_frame_lock);

				pthread_rwlock_unlock(&stream->lock);

				stream = stream->next;
			}

			pthread_rwlock_unlock(&dst_str_list->lock);
			
			have_new_frame = false;
		}
             
		gettimeofday(&finish, NULL);

		diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec)/1000; // In ms
	}

	stop_receiver(receiver);
	stop_out_manager();
	destroy_stream_list(src_str_list);
	destroy_stream_list(dst_str_list);

	destroy_participant_list(dst_p_list);
	destinations.clear();
	delete layout;

}

void mixer::init(uint32_t layout_width, uint32_t layout_height, uint32_t max_streams, uint32_t in_port, uint32_t out_port){
	layout = new Layout(layout_width, layout_height, PIX_FMT_RGB24, max_streams);
	input_str_list = init_stream_list();
	output_str_list = init_stream_list();
	src_p_list = init_participant_list();
	dst_p_list = init_participant_list();
	receiver = init_receiver(src_p_list, in_port);
	_in_port = in_port;
	_out_port = out_port;
	dst_counter = 0;
	max_frame_rate = 30;
}

void mixer::exec(){
	start_receiver(receiver);
	start_out_manager(dst_p_list, 25);
	pthread_create(&thread, NULL, mixer::execute_run, this);
}

void mixer::stop(){
	should_stop = true;
}

int mixer::add_source(uint32_t new_w, uint32_t new_h, uint32_t x, uint32_t y, uint32_t layer, codec_t codec){
	//TODO: generate ID for participant
	//TODO: maybe we can generate at this point the stream in order to clean receiver code
	pthread_rwlock_wrlock(&src_p_list->lock);
	int ret = add_participant(src_p_list, id, 0, 0, codec, NULL, 0, INPUT);
	pthread_rwlock_unlock(&src_p_list->lock);
	return ret;
}

int mixer::remove_source(uint32_t id){
	if (layout == NULL)
		return -1;
	layout->remove_stream(id);
	pthread_rwlock_wrlock(&src_p_list->lock);
	int ret = remove_participant(src_p_list, id);
	pthread_rwlock_unlock(&src_p_list->lock);
	return ret;
}

int mixer::add_destination(codec_t codec, char *ip, uint32_t port){
	if (layout == NULL)
		return -1;

	pthread_rwlock_wrlock(&dst_p_list->lock);
	int ret =  add_participant(dst_p_list, dst_counter, layout->get_w(), layout->get_h(), codec, ip, port, OUTPUT);
	if(ret != -1){
		Dst dest = {ip,port};
		destinations[dst_counter] = dest; 
	}
	pthread_rwlock_unlock(&dst_p_list->lock);
	dst_counter++;
	return ret;
}

int mixer::remove_destination(uint32_t id){
	if (layout == NULL)
		return -1;

	pthread_rwlock_wrlock(&dst_p_list->lock);
	int ret = remove_participant(dst_p_list, id);
	if(ret != -1){
		destinations.erase(id); 
	}
	pthread_rwlock_unlock(&dst_p_list->lock);
	return ret;
}

int mixer::modify_stream (uint32_t id, uint32_t width, uint32_t height, uint32_t x, uint32_t y, uint32_t layer, bool keep_aspect_ratio){
	if (layout == NULL)
		return -1;

	return layout->modify_stream(id, width, height, PIX_FMT_RGB24, x, y, layer, keep_aspect_ratio);
}

int mixer::resize_output (uint32_t width, uint32_t height, bool resize_streams){
	if (layout == NULL)
		return -1;

	return layout->modify_layout(width,height, PIX_FMT_RGB24, resize_streams);
}

void mixer::change_max_framerate(uint32_t frame_rate){
	max_frame_rate = frame_rate;
}

void mixer::get_stream_info(std::map<string, uint32_t> &str_map, uint32_t id){
	str_map["id"] = id;
	str_map["orig_width"] = layout->get_stream(id)->get_orig_w();
	str_map["orig_height"] = layout->get_stream(id)->get_orig_h();
	str_map["width"] = layout->get_stream(id)->get_curr_w();
	str_map["height"] = layout->get_stream(id)->get_curr_h();
	str_map["x"] = layout->get_stream(id)->get_x_pos();
	str_map["y"] = layout->get_stream(id)->get_y_pos();
	str_map["layer"] = layout->get_stream(id)->get_layer();
	str_map["active"] = (uint32_t)layout->get_stream(id)->get_active();
}

vector<uint32_t> mixer::get_streams_id(){
	if (layout == NULL)
		return std::vector<uint32_t>();

	return layout->get_streams_id();
}

int mixer::get_destination(int id, std::string &ip, int *port){
	if(destinations.count(id)>0){
		ip = destinations[id].ip;
		*port = destinations[id].port;
		return 0;
	}
	return -1;
}

map<uint32_t, mixer::Dst> mixer::get_destinations(){
	return destinations;
}

int mixer::set_stream_active(uint32_t id, uint8_t active_flag){
	if (layout == NULL)
		return -1;

	int i;
	struct participant_data* part;

	pthread_rwlock_wrlock(&src_p_list->lock);
	set_active_participant(get_participant_id(src_p_list, id), active_flag);
	layout->set_active(id, active_flag);
	pthread_rwlock_unlock(&src_p_list->lock);

	if (active_flag == 0){
		pthread_rwlock_rdlock(&dst_p_list->lock);

		part = dst_p_list->first;

		for (i=0; i<dst_p_list->count; i++){
			pthread_mutex_lock(&part->lock);
				
			memcpy((uint8_t*)part->frame, (uint8_t*)layout->get_layout_bytestream(), layout->get_buffsize());
			part->frame_length = layout->get_buffsize();
			part->new_frame = 1;
			pthread_mutex_unlock(&part->lock);

			part = part->next;
		}

		pthread_rwlock_unlock(&dst_p_list->lock);
	}
	
	return 0;
}

int mixer::get_layout_size(int *width, int *height){
	if (layout == NULL)
		return -1;

	*width = layout->get_w();
    *height = layout->get_h();
    return 0;
}

mixer::mixer(){}

mixer* mixer::get_instance(){
	if (mixer_instance == NULL){
		mixer_instance = new mixer();
	}
	return mixer_instance;
}

void* mixer::execute_run(void *context){
	return ((mixer *)context)->run();
}

uint8_t mixer::get_state(){
	return state;
}

void mixer::set_state(uint8_t s){
	state = s;
}
