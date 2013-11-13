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

		pthread_rwlock_rdlock(&src_str_list->lock);

		stream = src_str_list->first;

		for (i=0; i<src_str_list->count; i++){
			if (stream == NULL){
				continue;
			}

			pthread_rwlock_rdlock(&stream->lock);
			if(!layout->check_if_stream(stream->id) && stream->video->decoder != NULL){
				pthread_rwlock_rdlock(&stream->video->lock);
				layout->add_stream(stream->id, stream->video->decoded_frame->width, stream->video->decoded_frame->height);
				pthread_rwlock_unlock(&stream->video->lock);
			}

			pthread_mutex_lock(&stream->video->new_decoded_frame_lock);
			if (stream->video->new_decoded_frame){
				pthread_rwlock_rdlock(&stream->video->decoded_frame->lock);
				layout->introduce_frame_to_stream(stream->id, (uint8_t*)stream->video->decoded_frame->buffer, stream->video->decoded_frame->buffer_len);
				pthread_rwlock_unlock(&stream->video->decoded_frame->lock);
				have_new_frame = true;
				stream->video->new_decoded_frame = FALSE;
			}
			pthread_mutex_unlock(&stream->video->new_decoded_frame_lock);

			pthread_rwlock_unlock(&stream->lock);

			stream = stream->next;
		}

		pthread_rwlock_unlock(&src_str_list->lock);

		if (have_new_frame){
			layout->compose_layout();

			pthread_rwlock_rdlock(&dst_str_list->lock);

			stream = dst_str_list->first;

			for (i=0; i<dst_str_list->count; i++){
				pthread_rwlock_wrlock(&stream->video->decoded_frame->lock);
				memcpy
				(
					(uint8_t*)stream->video->decoded_frame->buffer, 
					(uint8_t*)layout->get_output_crop_buffer(stream->id), 
					layout->get_output_crop_buffer_size(stream->id)
				);
				pthread_rwlock_unlock(&stream->video->decoded_frame->lock);
				sem_post(&dst_str_list->first->video->encoder->input_sem);

				stream = stream->next;
			}

			pthread_rwlock_unlock(&dst_str_list->lock);

			have_new_frame = false;
		}
             
		gettimeofday(&finish, NULL);

		diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec)/1000; // In ms
	}

	stop_receiver(receiver);
	stop_transmitter(transmitter);
	destroy_stream_list(src_str_list);
	destroy_stream_list(dst_str_list);

	destinations.clear();
	delete layout;

}

void mixer::init(uint32_t layout_width, uint32_t layout_height, uint32_t in_port, uint32_t out_port){
	layout = new Layout(layout_width, layout_height);
	src_str_list = init_stream_list();
	dst_str_list = init_stream_list();
	receiver = init_receiver(src_str_list, in_port);
	transmitter = init_transmitter(dst_str_list, 25);
	_in_port = in_port;
	_out_port = out_port;
	dst_counter = 0;
	max_frame_rate = 30;
}

void mixer::exec(){
	start_receiver(receiver);
	start_transmitter(transmitter);
	pthread_create(&thread, NULL, mixer::execute_run, this);
}

void mixer::stop(){
	should_stop = true;
}

int mixer::add_source()
{
	uint32_t id = rand();
	return add_receiver_participant(receiver, id);
}

int mixer::remove_source(uint32_t id)
{
	uint32_t part_id;
	part_id = get_participant_from_stream_id(receiver->participant_list, id);
	if (part_id >= 0){
		remove_participant(receiver->participant_list, part_id);
	}
	remove_stream(src_str_list, id);
	layout->remove_stream(id);

	return TRUE;
}
		
int mixer::add_crop_to_source(uint32_t id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
   					     uint32_t layer, uint32_t rsz_width, uint32_t rsz_height, uint32_t rsz_x, uint32_t rsz_y)
{
	return layout->add_crop_to_stream(id, crop_width, crop_height, crop_x, crop_y, layer, rsz_width, rsz_height, rsz_x, rsz_y);

}

int mixer::modify_crop_from_source(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, 
       					      uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	return layout->modify_orig_crop_from_stream(stream_id, crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);
}

int mixer::modify_crop_resizing_from_source(uint32_t stream_id, uint32_t crop_id, uint32_t new_rsz_width, 
        							   uint32_t new_rsz_height, uint32_t new_rsz_x, uint32_t new_rsz_y, uint32_t new_layer)
{
	return layout->modify_dst_crop_from_stream(stream_id, crop_id, new_rsz_width, new_rsz_height, new_rsz_x, new_rsz_y, new_layer);
}

int mixer::remove_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	return layout->remove_crop_from_stream(stream_id, crop_id);
}

int mixer::add_crop_to_layout(uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t output_width, uint32_t output_height)
{
	uint32_t id = layout->add_crop_to_output_stream(crop_width, crop_height, crop_x, crop_y, output_width, output_height);

	if (id == 0){
		return FALSE;
	}

	stream_data_t *stream = init_stream(VIDEO, OUTPUT, id, ACTIVE, NULL);
    set_video_data_frame(stream->video->decoded_frame, RAW, crop_width, crop_height);
    set_video_data_frame(stream->video->coded_frame, H264, crop_width, crop_height);
    add_stream(dst_str_list, stream);

	return TRUE;
}

int mixer::modify_crop_from_layout(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	return layout->modify_crop_from_output_stream(crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);
}

int mixer::modify_crop_resizing_from_layout(uint32_t crop_id, uint32_t new_width, uint32_t new_height)
{
	//TODO: deactivated until encoder reconfigure implemented
	//return layout->modify_crop_resize_from_output_stream(crop_id, new_width, new_height);
	return FALSE;
}

int mixer::remove_crop_from_layout(uint32_t crop_id)
{
	if(layout->remove_crop_from_output_stream(crop_id)){
		remove_stream(dst_str_list, crop_id);
		return TRUE;
	} 

	return FALSE;
}

int mixer::add_destination(char *ip, uint32_t port, uint32_t stream_id)
{
	participant_data_t *participant = init_participant(dst_counter, OUTPUT, ip, port);

	stream_data_t *stream = get_stream_id(dst_str_list, stream_id);

	if (stream == NULL){
		return FALSE;
	}

	add_participant_stream(participant, stream);

	if(add_transmitter_participant(transmitter, participant)){
		Dst dest = {ip,port};
		destinations[dst_counter] = dest; 
		dst_counter++;
		return TRUE;
	}
		
	return FALSE;	
}

int mixer::remove_destination(uint32_t id)
{
	if(destroy_transmitter_participant(transmitter, id)){
		destinations.erase(id);
		return TRUE;
	}

	return FALSE;
}

int mixer::enable_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	return layout->enable_crop_from_stream(stream_id, crop_id);
}

int mixer::disable_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	return layout->disable_crop_from_stream(stream_id, crop_id);
}

void mixer::change_max_framerate(uint32_t frame_rate){
	max_frame_rate = frame_rate;
}

// void mixer::get_stream_info(std::map<string, uint32_t> &str_map, uint32_t id){
// 	str_map["id"] = id;
// 	str_map["orig_width"] = layout->get_stream(id)->get_orig_w();
// 	str_map["orig_height"] = layout->get_stream(id)->get_orig_h();
// 	str_map["width"] = layout->get_stream(id)->get_curr_w();
// 	str_map["height"] = layout->get_stream(id)->get_curr_h();
// 	str_map["x"] = layout->get_stream(id)->get_x_pos();
// 	str_map["y"] = layout->get_stream(id)->get_y_pos();
// 	str_map["layer"] = layout->get_stream(id)->get_layer();
// 	str_map["active"] = (uint32_t)layout->get_stream(id)->get_active();
// }

// vector<uint32_t> mixer::get_streams_id(){
// 	if (layout == NULL)
// 		return std::vector<uint32_t>();

// 	return layout->get_streams_id();
// }

// int mixer::get_destination(int id, std::string &ip, int *port){
// 	if(destinations.count(id)>0){
// 		ip = destinations[id].ip;
// 		*port = destinations[id].port;
// 		return 0;
// 	}
// 	return -1;
// }

// map<uint32_t, mixer::Dst> mixer::get_destinations(){
// 	return destinations;
// }

// int mixer::change_stream_state(uint32_t id, stream_state_t state){
// 	if (layout == NULL)
// 		return -1;
// 	stream_data_t *stream;

// 	stream = get_stream_id(src_str_list, id);

// 	if (stream == NULL)
// 		return -1;

// 	set_stream_state(stream, state);
// 	if (state == ACTIVE){
// 		layout->set_active(id, 1);
// 	} else if (state == NON_ACTIVE){
// 		layout->set_active(id, 0);
// 	}
	
// 	return 0;
// }

// int mixer::get_layout_size(int *width, int *height){
// 	if (layout == NULL)
// 		return -1;

// 	*width = layout->get_w();
//     *height = layout->get_h();
//     return 0;
// }

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
