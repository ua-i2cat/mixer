 /*
 *  MIXER - A real-time video mixing application
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin MIXER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
 *			  Ignacio Contreras <ignacio.contreras@i2cat.net>
 */		

#include <iostream>
#include <map>
#include <string>
#include "mixer.h"
#include <sys/time.h>

using namespace std;
Mixer* Mixer::mixer_instance;

void* Mixer::run(void) {
	int i;
	stream_data_t *stream;
	have_new_frame = false;
	should_stop = false;
	struct timeval start, finish;
	long diff = 0, min_diff = 0;

	min_diff = ((float)1/(float)max_frame_rate)*1000000; // In ms

	while (!should_stop){
	    min_diff = ((float)1/(float)max_frame_rate)*1000000;

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

			if (stream->video->new_decoded_frame){
				pthread_rwlock_rdlock(&stream->video->decoded_frame->lock);
				layout->introduce_frame_to_stream(stream->id, (uint8_t*)stream->video->decoded_frame->buffer, stream->video->decoded_frame->buffer_len);
				pthread_rwlock_unlock(&stream->video->decoded_frame->lock);
				have_new_frame = true;
				stream->video->new_decoded_frame = FALSE;
			}
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
				sem_post(&stream->video->encoder->input_sem);
				stream->video->new_decoded_frame = TRUE;

				stream = stream->next;
			}

			pthread_rwlock_unlock(&dst_str_list->lock);

			have_new_frame = false;
		}
             
		gettimeofday(&finish, NULL);

		diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec); // In ms

		if (diff < min_diff){
			usleep(min_diff - diff); 
		}
	}

	stop_receiver(receiver);
	stop_transmitter(transmitter);
	destroy_stream_list(src_str_list);
	destroy_stream_list(dst_str_list);

	delete layout;

}

void Mixer::init(uint32_t layout_width, uint32_t layout_height, uint32_t in_port){
	layout = new Layout(layout_width, layout_height);
	src_str_list = init_stream_list();
	dst_str_list = init_stream_list();
	uint32_t id = layout->add_crop_to_output_stream(layout_width, layout_height, 0, 0, layout_width, layout_height);
	stream_data_t *stream = init_stream(VIDEO, OUTPUT, id, ACTIVE, NULL);
    set_video_data_frame(stream->video->decoded_frame, RAW, layout_width, layout_height);
    set_video_data_frame(stream->video->coded_frame, H264, layout_width, layout_height);
    add_stream(dst_str_list, stream);
    init_encoder(stream->video);
	receiver = init_receiver(src_str_list, in_port);
	transmitter = init_transmitter(dst_str_list, 25);
	_in_port = in_port;
	dst_counter = 0;
	max_frame_rate = 30;
}

void Mixer::exec(){
	start_receiver(receiver);
	start_transmitter(transmitter);
	pthread_create(&thread, NULL, Mixer::execute_run, this);
}

void Mixer::stop(){
	should_stop = true;
}

int Mixer::add_source()
{
	uint32_t id = rand();
	int ret = add_receiver_participant(receiver, id);
	if (ret == FALSE){
		return ret;
	}

	return id;
}

int Mixer::remove_source(uint32_t id)
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
		
int Mixer::add_crop_to_source(uint32_t id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
   					     uint32_t layer, uint32_t rsz_width, uint32_t rsz_height, uint32_t rsz_x, uint32_t rsz_y)
{
	return layout->add_crop_to_stream(id, crop_width, crop_height, crop_x, crop_y, layer, rsz_width, rsz_height, rsz_x, rsz_y);

}

int Mixer::modify_crop_from_source(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, 
       					      uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	return layout->modify_orig_crop_from_stream(stream_id, crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);
}

int Mixer::modify_crop_resizing_from_source(uint32_t stream_id, uint32_t crop_id, uint32_t new_rsz_width, 
        							   uint32_t new_rsz_height, uint32_t new_rsz_x, uint32_t new_rsz_y, uint32_t new_layer)
{
	return layout->modify_dst_crop_from_stream(stream_id, crop_id, new_rsz_width, new_rsz_height, new_rsz_x, new_rsz_y, new_layer);
}

int Mixer::remove_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	return layout->remove_crop_from_stream(stream_id, crop_id);
}

int Mixer::add_crop_to_layout(uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t output_width, uint32_t output_height)
{
	uint32_t id = layout->add_crop_to_output_stream(crop_width, crop_height, crop_x, crop_y, output_width, output_height);

	if (id == 0){
		return FALSE;
	}

	stream_data_t *stream = init_stream(VIDEO, OUTPUT, id, ACTIVE, NULL);
    set_video_data_frame(stream->video->decoded_frame, RAW, crop_width, crop_height);
    set_video_data_frame(stream->video->coded_frame, H264, crop_width, crop_height);
    add_stream(dst_str_list, stream);
    init_encoder(stream->video);

	return TRUE;
}

int Mixer::modify_crop_from_layout(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	return layout->modify_crop_from_output_stream(crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);
}

int Mixer::modify_crop_resizing_from_layout(uint32_t crop_id, uint32_t new_width, uint32_t new_height)
{
	//TODO: deactivated until encoder reconfigure implemented
	//return layout->modify_crop_resize_from_output_stream(crop_id, new_width, new_height);
	return FALSE;
}

int Mixer::remove_crop_from_layout(uint32_t crop_id)
{
	if(layout->remove_crop_from_output_stream(crop_id)){
		remove_stream(dst_str_list, crop_id);
		return TRUE;
	} 

	return FALSE;
}

int Mixer::add_destination(char *ip, uint32_t port, uint32_t stream_id)
{
	participant_data_t *participant = init_participant(dst_counter, OUTPUT, ip, port);

	stream_data_t *stream = get_stream_id(dst_str_list, stream_id);

	if (stream == NULL){
		return FALSE;
	}

	add_participant_stream(participant, stream);

	if(add_transmitter_participant(transmitter, participant)){
		dst_counter++;
		return TRUE;
	}
	
	return FALSE;	
}

int Mixer::remove_destination(uint32_t id)
{
	if(destroy_transmitter_participant(transmitter, id)){
		return TRUE;
	}

	return FALSE;
}

int Mixer::enable_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	return layout->enable_crop_from_stream(stream_id, crop_id);
}

int Mixer::disable_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	return layout->disable_crop_from_stream(stream_id, crop_id);
}

void Mixer::change_max_framerate(uint32_t frame_rate){
	max_frame_rate = frame_rate;
}

Layout* Mixer::get_layout()
{
	return layout;
}

vector<Mixer::Dst>* Mixer::get_destinations()
{
	participant_data_t *participant;
	struct Dst dst;
	std::vector<Mixer::Dst> vect;
	pthread_rwlock_rdlock(&transmitter->participants->lock);
	participant = transmitter->participants->first;

	while(participant != NULL){
		dst.id = participant->id;
		dst.ip = participant->rtp.addr;
		dst.port = participant->rtp.port;
		dst.stream_id = participant->stream->id;
		vect.push_back(dst);
		participant = participant->next;
	}

	pthread_rwlock_unlock(&transmitter->participants->lock);
	return &vect;
}

Mixer::Mixer(){}

Mixer* Mixer::get_instance(){
	if (mixer_instance == NULL){
		mixer_instance = new Mixer();
	}
	return mixer_instance;
}

void* Mixer::execute_run(void *context){
	return ((Mixer *)context)->run();
}

uint8_t Mixer::get_state(){
	return state;
}

void Mixer::set_state(uint8_t s){
	state = s;
}
