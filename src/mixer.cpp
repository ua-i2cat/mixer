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
#include <io_mngr/tv.h>
#include "../config.h"

#define DEF_FPS 30

using namespace std;
Mixer* Mixer::mixer_instance;

void* Mixer::run(void) {
	should_stop = false;
	struct timeval start, finish;
	long diff = 0, min_diff = 0;

#ifdef STATS
	uint32_t compose_time;
#endif

	min_diff = ((float)1/(float)max_frame_rate)*1000000; // In ms

	while (!should_stop){
		pthread_rwlock_wrlock(&task_lock);
	    min_diff = ((float)1/(float)max_frame_rate)*1000000;

		gettimeofday(&start, NULL);

		if (!receive_frames()){
			pthread_rwlock_unlock(&task_lock);

			gettimeofday(&finish, NULL);
			diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec); // In ms
			if (diff < min_diff){
				usleep(min_diff - diff); 
			}

			continue;
		}

		layout->resize_input_crops();
		update_input_frames();
#ifdef STATS
		compose_time = get_local_mediatime_us();
#endif
		layout->compose_layout();
#ifdef STATS
		compose_time = get_local_mediatime_us() - compose_time;
#endif
		update_output_frame_buffers();
		layout->split_layout();
		update_output_frames();

		gettimeofday(&finish, NULL);

		diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec); // In ms

#ifdef STATS
		s_mng->update_mix_stat(compose_time);
#endif

		pthread_rwlock_unlock(&task_lock);
		if (diff < min_diff){
			usleep(min_diff - diff); 
		}
	}

	stop_receiver(receiver);
	stop_transmitter(transmitter);
	destroy_stream_list(in_video_str);
	destroy_stream_list(out_video_str);

	delete layout;

}

int Mixer::receive_frames()
{
	int i=0;
	stream_data_t *stream;
	video_data_frame_t *decoded_frame;
	int ret = FALSE;

	pthread_rwlock_rdlock(&in_video_str->lock);
	stream = in_video_str->first;

	for (i=0; i<in_video_str->count; i++){
		decoded_frame = curr_out_frame(stream->video->decoded_frames);
		if (decoded_frame == NULL){
			stream = stream->next;
            continue;
        }

#ifdef STATS
        s_mng->update_input_stat(stream->id, 
        						 stream->video->coded_frames->delay, 
        						 stream->video->decoded_frames->delay, 
        						 stream->video->coded_frames->fps,
        						 decoded_frame->seqno, 
        						 stream->video->lost_coded_frames,
        						 stream->video->bitrate);
#endif

		if (!layout->check_if_stream_init(stream->id) && stream->video->decoder != NULL){
			layout->init_stream(stream->id, decoded_frame->width, decoded_frame->height);
		}

		layout->introduce_frame_to_stream(stream->id, (uint8_t*)decoded_frame->buffer, decoded_frame->buffer_len);
		ret = TRUE;
		stream = stream->next;
	}
	pthread_rwlock_unlock(&in_video_str->lock);

	return ret;
}

void Mixer::update_input_frames()
{
	int i = 0;
	stream_data_t *stream;

	pthread_rwlock_rdlock(&in_video_str->lock);
	stream = in_video_str->first;
	for (i=0; i<in_video_str->count; i++){
		remove_frame(stream->video->decoded_frames);
		stream = stream->next;
	}
	pthread_rwlock_unlock(&in_video_str->lock);
}

void Mixer::update_output_frames()
{
	int i = 0;
	stream_data_t *stream;

	pthread_rwlock_rdlock(&out_video_str->lock);
	stream = out_video_str->first;
	for (i=0; i<out_video_str->count; i++){
#ifdef STATS
		s_mng->update_output_stat(stream->id,
								  stream->video->decoded_frames->delay, 
								  stream->video->coded_frames->delay,
								  stream->video->decoded_frames->fps,
								  stream->video->seqno,
								  stream->video->lost_coded_frames);
#endif
		put_frame(stream->video->decoded_frames);
		stream = stream->next;
	}
	pthread_rwlock_unlock(&out_video_str->lock);
}

void Mixer::update_output_frame_buffers()
{
	int i = 0;
	stream_data_t *stream;
	video_data_frame_t *decoded_frame;

	pthread_rwlock_rdlock(&out_video_str->lock);
	stream = out_video_str->first;

	for (i=0; i<out_video_str->count; i++){
		decoded_frame = curr_in_frame(stream->video->decoded_frames);
    	while (decoded_frame == NULL){
    		flush_frames(stream->video->decoded_frames);
#ifdef STATS
    		s_mng->output_frame_lost(stream->id);
#endif
            decoded_frame = curr_in_frame(stream->video->decoded_frames);
    	}
    	stream->video->seqno++;
    	decoded_frame->seqno = stream->video->seqno;
    	decoded_frame->media_time = get_local_mediatime_us();
    	layout->set_resized_output_buffer(stream->id, decoded_frame->buffer);
		stream = stream->next;
	}
	pthread_rwlock_unlock(&out_video_str->lock);
}

void Mixer::init(uint32_t layout_width, uint32_t layout_height, uint32_t in_port){
	layout = new Layout(layout_width, layout_height);
	in_video_str = init_stream_list();
	out_video_str = init_stream_list();
	in_audio_str = init_stream_list();
	out_audio_str = init_stream_list();

	uint32_t id = layout->add_crop_to_output_stream(layout_width, layout_height, 0, 0, layout_width, layout_height);
	stream_data_t *stream = init_stream(VIDEO, OUTPUT, id, ACTIVE, DEF_FPS , NULL);
    set_video_frame_cq(stream->video->decoded_frames, RAW, layout_width, layout_height);
    set_video_frame_cq(stream->video->coded_frames, H264, layout_width, layout_height);
    add_stream(out_video_str, stream);
    init_encoder(stream->video);

	receiver = init_receiver(in_video_str, in_audio_str, in_port, in_port + 2);
	transmitter = init_transmitter(out_video_str, out_audio_str, DEF_FPS);
	_in_port = in_port;
	max_frame_rate = DEF_FPS;
	pthread_rwlock_init(&task_lock, NULL);

#ifdef STATS
	s_mng = new statManager();
    s_mng->add_output_stream(id);
#endif

}

void Mixer::exec(){
	start_receiver(receiver);
	start_transmitter(transmitter);
	pthread_create(&thread, NULL, Mixer::execute_run, this);
}

void Mixer::stop(){
	should_stop = true;
}

uint32_t Mixer::add_source()
{
	pthread_rwlock_wrlock(&task_lock);
	uint32_t id = rand();
	participant_data *participant = init_participant(id, INPUT, NULL, 0);
  	
    stream_data_t *stream = init_stream(VIDEO, INPUT, id, I_AWAIT, DEF_FPS, NULL);
    set_video_frame_cq(stream->video->coded_frames, H264, 0, 0);
    add_participant_stream(stream, participant);
    add_stream(in_video_str, stream);

#ifdef STATS
    s_mng->add_input_stream(id);
#endif

    layout->add_stream(id);

    pthread_rwlock_unlock(&task_lock);
	return id;
}

int Mixer::remove_source(uint32_t id)
{
	pthread_rwlock_wrlock(&task_lock);

	if(!layout->remove_stream(id)){
		pthread_rwlock_unlock(&task_lock);
		return FALSE;
	}

	if(!remove_stream(in_video_str, id)){
		pthread_rwlock_unlock(&task_lock);
		return FALSE;
	}

#ifdef STATS
	s_mng->remove_input_stream(id);
#endif

	pthread_rwlock_unlock(&task_lock);
	return TRUE;
}
		
int Mixer::add_crop_to_source(uint32_t id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
   					     uint32_t layer, uint32_t rsz_width, uint32_t rsz_height, uint32_t rsz_x, uint32_t rsz_y)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret = layout->add_crop_to_stream(id, crop_width, crop_height, crop_x, crop_y, layer, rsz_width, rsz_height, rsz_x, rsz_y);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

int Mixer::modify_crop_from_source(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, 
       					      uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret = layout->modify_orig_crop_from_stream(stream_id, crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

int Mixer::modify_crop_resizing_from_source(uint32_t stream_id, uint32_t crop_id, uint32_t new_rsz_width, 
        							   uint32_t new_rsz_height, uint32_t new_rsz_x, uint32_t new_rsz_y, uint32_t new_layer)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret =  layout->modify_dst_crop_from_stream(stream_id, crop_id, new_rsz_width, new_rsz_height, new_rsz_x, new_rsz_y, new_layer);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

int Mixer::remove_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret = layout->remove_crop_from_stream(stream_id, crop_id);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

int Mixer::add_crop_to_layout(uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t output_width, uint32_t output_height)
{
	pthread_rwlock_wrlock(&task_lock);
	uint32_t id = layout->add_crop_to_output_stream(crop_width, crop_height, crop_x, crop_y, output_width, output_height);
	if (id == 0){
		pthread_rwlock_unlock(&task_lock);
		return FALSE;
	}

    stream_data_t *stream = init_stream(VIDEO, OUTPUT, id, ACTIVE, DEF_FPS, NULL);
    set_video_frame_cq(stream->video->decoded_frames, RAW, crop_width, crop_height);
    set_video_frame_cq(stream->video->coded_frames, H264, crop_width, crop_height);
    add_stream(out_video_str, stream);
    init_encoder(stream->video);

#ifdef STATS
    s_mng->add_output_stream(id);
#endif

    pthread_rwlock_unlock(&task_lock);
	return TRUE;
}

int Mixer::modify_crop_from_layout(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret =  layout->modify_crop_from_output_stream(crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

int Mixer::modify_crop_resizing_from_layout(uint32_t id, uint32_t new_width, uint32_t new_height)
{
	pthread_rwlock_wrlock(&task_lock);
	if(layout->modify_crop_resize_from_output_stream(id, new_width, new_height) == FALSE){
		pthread_rwlock_unlock(&task_lock);
		return FALSE;
	}

	stream_data_t *stream = get_stream_id(out_video_str, id);

	while(stream->video->decoded_frames->state != CQ_EMPTY){
		usleep(500);	//TODO: GET RID OF MAGIC NUMBERS	
	}
	set_video_frame_cq(stream->video->decoded_frames, RAW, new_width, new_height);

	while(stream->video->coded_frames->state != CQ_EMPTY){
		usleep(500);	//TODO: GET RID OF MAGIC NUMBERS	
	}
    set_video_frame_cq(stream->video->coded_frames, H264, new_width, new_height);

	pthread_rwlock_unlock(&task_lock);
	return TRUE;
}

int Mixer::remove_crop_from_layout(uint32_t crop_id)
{
	pthread_rwlock_wrlock(&task_lock);
	if(layout->remove_crop_from_output_stream(crop_id)){
		remove_stream(out_video_str, crop_id);
#ifdef STATS
    	s_mng->remove_output_stream(crop_id);
#endif
		pthread_rwlock_unlock(&task_lock);
		return TRUE;
	} 

	pthread_rwlock_unlock(&task_lock);
	return FALSE;
}

uint32_t Mixer::add_destination(char *ip, uint32_t port, uint32_t stream_id)
{
	pthread_rwlock_wrlock(&task_lock);

	stream_data_t *stream = get_stream_id(out_video_str, stream_id);

	if (stream == NULL){
		pthread_rwlock_unlock(&task_lock);
		return FALSE;
	}
	
	uint32_t id = rand();	
	participant_data_t *participant = init_participant(id, OUTPUT, ip, port);
	add_participant_stream(stream, participant);

	pthread_rwlock_unlock(&task_lock);
	return id;	
}

int Mixer::remove_destination(uint32_t id)
{
	int i = 0;
	stream_data_t* stream;
	pthread_rwlock_wrlock(&task_lock);

	pthread_rwlock_rdlock(&out_video_str->lock);
	stream = out_video_str->first;

	for (i=0; i<out_video_str->count; i++){
		if (remove_participant_from_stream(stream, id)){
			pthread_rwlock_unlock(&out_video_str->lock);
			pthread_rwlock_unlock(&task_lock);
			return TRUE;
		}
	}
	pthread_rwlock_unlock(&out_video_str->lock);
	pthread_rwlock_unlock(&task_lock);
	return FALSE;
}

vector<Mixer::Dst> Mixer::get_output_stream_destinations(uint32_t id)
{
	int i;
	participant_data_t *participant;
	stream_data_t *stream;
	struct Dst dst;
	std::vector<Mixer::Dst> vect;

	pthread_rwlock_rdlock(&task_lock);
	stream = get_stream_id(out_video_str, id);

	if (stream == NULL){
		pthread_rwlock_unlock(&task_lock);
		return vect;
	}

	pthread_rwlock_rdlock(&stream->plist->lock);
	participant = stream->plist->first;

	for (i=0; i<stream->plist->count; i++){
		dst.id = participant->id;
		dst.ip = participant->rtp->addr;
		dst.port = participant->rtp->port;
		vect.push_back(dst);
		participant = participant->next;
	}

	pthread_rwlock_unlock(&stream->plist->lock);
	pthread_rwlock_unlock(&task_lock);
	return vect;
}

int Mixer::enable_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret =  layout->enable_crop_from_stream(stream_id, crop_id);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

int Mixer::disable_crop_from_source(uint32_t stream_id, uint32_t crop_id)
{
	pthread_rwlock_wrlock(&task_lock);
	int ret = layout->disable_crop_from_stream(stream_id, crop_id);
	pthread_rwlock_unlock(&task_lock);

	return ret;
}

Layout* Mixer::get_layout()
{
	return layout;
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

uint32_t Mixer::get_layout_width()
{
	return layout->get_out_stream()->get_width();
}

uint32_t Mixer::get_layout_height()
{
	return layout->get_out_stream()->get_height();
}

pthread_rwlock_t* Mixer::get_task_lock()
{
	return &task_lock;
}

void Mixer::get_stats(map<uint32_t,streamStats*> &input_stats, map<uint32_t,streamStats*> &output_stats)
{
	s_mng->get_stats(input_stats, output_stats);
}



