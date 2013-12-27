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

void* Mixer::main_routine(void) {
	should_stop = false;
	struct timeval start, finish, curr_time;
	long diff = 0, min_diff = 0;
    uint32_t curr_ts;

#ifdef STATS
	uint32_t compose_time;
#endif

	min_diff = 1000000/max_frame_rate; // In us

	while (!should_stop){
	    min_diff = 1000000/max_frame_rate;

	    gettimeofday(&curr_time, NULL);
        curr_ts = curr_time.tv_sec*1000000 + curr_time.tv_usec;

        pthread_mutex_lock(&eventQueue_lock);
        if (!eventQueue.empty()){
            Event tmp = eventQueue.top();
            if (tmp.get_timestamp() <= curr_ts){ //TODO: this can be a while if we want to execute N orders in one iteration
            	tmp.exec_func(this);
            	tmp.send_and_close();
            	eventQueue.pop();
            }
        }
        pthread_mutex_unlock(&eventQueue_lock);

		gettimeofday(&start, NULL);

		if (!receive_frames()){

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

		if (diff < min_diff){
			usleep(min_diff - diff); 
		}
	}

	stop_receiver(receiver);
	stop_transmitter(transmitter);
	destroy_stream_list(in_video_str);
	destroy_stream_list(out_video_str);

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
								  stream->video->coded_frames->fps,
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

Mixer::Mixer(int layout_width, int layout_height, int in_port)
{
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
    pthread_mutex_init(&eventQueue_lock, NULL);

#ifdef STATS
	s_mng = new statManager();
    s_mng->add_output_stream(id);
#endif

}

void Mixer::start(){
	start_receiver(receiver);
	start_transmitter(transmitter);
	pthread_create(&thread, NULL, Mixer::execute_routine, this);
}

void Mixer::stop(){
	should_stop = true;
	pthread_join(thread, NULL);
}

Mixer::~Mixer()
{
    delete layout;

    #ifdef STATS
        delete s_mng;
    #endif
}

void Mixer::add_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int id = rand();
	participant_data *participant = init_participant(id, INPUT, NULL, 0);
  	
    stream_data_t *stream = init_stream(VIDEO, INPUT, id, I_AWAIT, DEF_FPS, NULL);
    set_video_frame_cq(stream->video->coded_frames, H264, 0, 0);
    add_participant_stream(stream, participant);
    add_stream(in_video_str, stream);

#ifdef STATS
    s_mng->add_input_stream(id);
#endif

    layout->add_stream(id);

    outRootNode->Add("id", id);
}

void Mixer::remove_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int id = params->Get("id").ToInt();

	if(!layout->remove_stream(id)){
        outRootNode->Add("error", "Error removing stream from layout. Check introduced ID");
		return;
	}

	if(!remove_stream(in_video_str, id)){
        outRootNode->Add("error", "Error removing stream from list. Check introduced ID");
		return;
	}

#ifdef STATS
	s_mng->remove_input_stream(id);
#endif
    outRootNode->Add("error", Jzon::null);
}
		
void Mixer::add_crop_to_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int id = params->Get("id").ToInt();
    int crop_width = params->Get("crop_width").ToInt();
    int crop_height = params->Get("crop_height").ToInt();
    int crop_x = params->Get("crop_x").ToInt();
    int crop_y = params->Get("crop_y").ToInt();
    int layer = params->Get("layer").ToInt();
    int rsz_width = params->Get("rsz_width").ToInt();
    int rsz_height = params->Get("rsz_height").ToInt();
    int rsz_x = params->Get("rsz_x").ToInt();
    int rsz_y = params->Get("rsz_y").ToInt();

	int ret = layout->add_crop_to_stream(id, crop_width, crop_height, crop_x, crop_y, layer, rsz_width, rsz_height, rsz_x, rsz_y);

    if (ret == FALSE){
        outRootNode->Add("error", "Error adding new crop. Check introduced parameters");
        return;
    }

    outRootNode->Add("error", Jzon::null);
}

void Mixer::modify_crop_from_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int stream_id = params->Get("stream_id").ToInt();
    int crop_id = params->Get("crop_id").ToInt();
    int new_crop_width = params->Get("width").ToInt();
    int new_crop_height = params->Get("height").ToInt();
    int new_crop_x = params->Get("x").ToInt();
    int new_crop_y = params->Get("y").ToInt();

	int ret = layout->modify_orig_crop_from_stream(stream_id, crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);

    if (ret == FALSE){
        outRootNode->Add("error", "Error modifying crop. Check introduced parameters");
        return;
    }

    outRootNode->Add("error", Jzon::null);

}

void Mixer::modify_crop_resizing_from_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int stream_id = params->Get("stream_id").ToInt();
    int crop_id = params->Get("crop_id").ToInt();
    int new_rsz_width = params->Get("width").ToInt();
    int new_rsz_height = params->Get("height").ToInt();
    int new_rsz_x = params->Get("x").ToInt();
    int new_rsz_y = params->Get("y").ToInt();
    int new_layer = params->Get("layer").ToInt();

	int ret = layout->modify_dst_crop_from_stream(stream_id, crop_id, new_rsz_width, new_rsz_height, new_rsz_x, new_rsz_y, new_layer);

    if (ret == FALSE){
        outRootNode->Add("error", "Error modifying crop. Check introduced parameters");
        return;
    }

    outRootNode->Add("error", Jzon::null);

}

void Mixer::remove_crop_from_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int stream_id = params->Get("stream_id").ToInt();
    int crop_id = params->Get("crop_id").ToInt();

	int ret = layout->remove_crop_from_stream(stream_id, crop_id);

    if (ret == FALSE){
        outRootNode->Add("error", "Error removing crop. Check introduced parameters");
        return;
    }

    outRootNode->Add("error", Jzon::null);

}

void Mixer::enable_crop_from_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int stream_id = params->Get("stream_id").ToInt();
    int crop_id = params->Get("crop_id").ToInt();

	int ret =  layout->enable_crop_from_stream(stream_id, crop_id);

    if (ret == FALSE){
        outRootNode->Add("error", "Error enabling crop. Wrong ID or maybe still enabled");
        return;
    }

    outRootNode->Add("error", Jzon::null);
}

void Mixer::disable_crop_from_source(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int stream_id = params->Get("stream_id").ToInt();
    int crop_id = params->Get("crop_id").ToInt();
	
	int ret = layout->disable_crop_from_stream(stream_id, crop_id);

    if (ret == FALSE){
        outRootNode->Add("error", "Error disabling crop. Wrong ID or maybe still disabled");
        return;
    }

    outRootNode->Add("error", Jzon::null);
}

void Mixer::add_crop_to_layout(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int crop_width = params->Get("width").ToInt();
    int crop_height = params->Get("height").ToInt();
    int crop_x = params->Get("x").ToInt();
    int crop_y = params->Get("y").ToInt();
    int output_width = params->Get("output_width").ToInt();
    int output_height = params->Get("output_height").ToInt();

	uint32_t id = layout->add_crop_to_output_stream(crop_width, crop_height, crop_x, crop_y, output_width, output_height);
	if (id == 0){
        outRootNode->Add("error", "Error adding new crop to layout. Check introduced parameters");
		return;
	}

    stream_data_t *stream = init_stream(VIDEO, OUTPUT, id, ACTIVE, DEF_FPS, NULL);
    set_video_frame_cq(stream->video->decoded_frames, RAW, output_width, output_height);
    set_video_frame_cq(stream->video->coded_frames, H264, output_width, output_height);
    add_stream(out_video_str, stream);
    init_encoder(stream->video);

#ifdef STATS
    s_mng->add_output_stream(id);
#endif

    outRootNode->Add("error", Jzon::null);

}

void Mixer::modify_crop_from_layout(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int crop_id = params->Get("crop_id").ToInt();
    int new_crop_width = params->Get("width").ToInt();
    int new_crop_height = params->Get("height").ToInt();
    int new_crop_x = params->Get("x").ToInt();
    int new_crop_y = params->Get("y").ToInt();

	int ret =  layout->modify_crop_from_output_stream(crop_id, new_crop_width, new_crop_height, new_crop_x, new_crop_y);

    if (ret == FALSE){
        outRootNode->Add("error", "Error modifying crop. Check introduced parameters");
        return;
    }

    outRootNode->Add("error", Jzon::null);
}

void Mixer::modify_crop_resizing_from_layout(Jzon::Object* params, Jzon::Object* outRootNode)
{
	int id = params->Get("crop_id").ToInt();
    int new_width = params->Get("width").ToInt();
    int new_height = params->Get("height").ToInt();

	if(layout->modify_crop_resize_from_output_stream(id, new_width, new_height) == FALSE){
        outRootNode->Add("error", "Error modifying crop. Check introduced parameters");
		return;
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

    outRootNode->Add("error", Jzon::null);
}

void Mixer::remove_crop_from_layout(Jzon::Object* params, Jzon::Object* outRootNode)
{
    int crop_id = params->Get("crop_id").ToInt();

	if(layout->remove_crop_from_output_stream(crop_id) == FALSE){
        outRootNode->Add("error", "Error removing stream from layout. Check introduced ID");
        return;
    } 

	if(remove_stream(out_video_str, crop_id) == FALSE){
        outRootNode->Add("error", "Error removing stream from list. Check introduced ID");
        return;
    }   

#ifdef STATS
    	s_mng->remove_output_stream(crop_id);
#endif
        outRootNode->Add("error", Jzon::null);
}

void Mixer::add_destination(Jzon::Object* params, Jzon::Object* outRootNode)
{
    int stream_id = params->Get("stream_id").ToInt();
    std::string ip_string = params->Get("ip").ToString();
    uint32_t port = params->Get("port").ToInt();
    char *ip = new char[ip_string.length() + 1];
    strcpy(ip, ip_string.c_str());

	stream_data_t *stream = get_stream_id(out_video_str, stream_id);

	if (stream == NULL){
        outRootNode->Add("error", "Introduced ID not valid");
		return;
	}
	
	uint32_t id = rand();	
	participant_data_t *participant = init_participant(id, OUTPUT, ip, port);
	add_participant_stream(stream, participant);

    outRootNode->Add("error", Jzon::null);
}

void Mixer::remove_destination(Jzon::Object* params, Jzon::Object* outRootNode)
{
	uint32_t id = params->Get("id").ToInt();

	int i = 0;
	stream_data_t* stream;

	pthread_rwlock_rdlock(&out_video_str->lock);
	stream = out_video_str->first;

	for (i=0; i<out_video_str->count; i++){
		if (remove_participant_from_stream(stream, id)){
			pthread_rwlock_unlock(&out_video_str->lock);
            outRootNode->Add("error", Jzon::null);
			return;
		}
	}
	pthread_rwlock_unlock(&out_video_str->lock);
    outRootNode->Add("error", "Introduced ID not valid");
}

vector<Mixer::Dst> Mixer::get_output_stream_destinations(uint32_t id)
{
	int i;
	participant_data_t *participant;
	stream_data_t *stream;
	struct Dst dst;
	std::vector<Mixer::Dst> vect;

	stream = get_stream_id(out_video_str, id);

	if (stream == NULL){
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
	return vect;
}

void* Mixer::execute_routine(void *context){
	return ((Mixer *)context)->main_routine();
}

void Mixer::push_event(Event e)
{
    pthread_mutex_lock(&eventQueue_lock);
    eventQueue.push(e);
    pthread_mutex_unlock(&eventQueue_lock);
}

void Mixer::get_streams(Jzon::Object* params, Jzon::Object* outRootNode){

    Jzon::Array stream_list;
    if(layout->get_streams()->empty()){
        outRootNode->Add("input_streams", stream_list);
        return;
    }

    std::map<uint32_t, Stream*>* stream_map;
    std::map<uint32_t, Crop*>* crop_map;
    std::map<uint32_t, Stream*>::iterator stream_it;
    std::map<uint32_t, Crop*>::iterator crop_it;

    stream_map = layout->get_streams();
    
    for (stream_it = stream_map->begin(); stream_it != stream_map->end(); stream_it++){
        Jzon::Object stream;
        Jzon::Array crop_list;
        stream.Add("id", (int)stream_it->second->get_id());
        stream.Add("width", (int)stream_it->second->get_width());
        stream.Add("height", (int)stream_it->second->get_height());
        crop_map = stream_it->second->get_crops();
        for (crop_it = crop_map->begin(); crop_it != crop_map->end(); crop_it++){
            Jzon::Object crop;
            crop.Add("id", (int)crop_it->second->get_id());
            crop.Add("c_w", (int)crop_it->second->get_crop_width());
            crop.Add("c_h", (int)crop_it->second->get_crop_height());
            crop.Add("c_x", (int)crop_it->second->get_crop_x());
            crop.Add("c_y", (int)crop_it->second->get_crop_y());
            crop.Add("dst_w", (int)crop_it->second->get_dst_width());
            crop.Add("dst_h", (int)crop_it->second->get_dst_height());
            crop.Add("dst_x", (int)crop_it->second->get_dst_x());
            crop.Add("dst_y", (int)crop_it->second->get_dst_y());
            crop.Add("layer", (int)crop_it->second->get_layer());
            crop.Add("state", (int)crop_it->second->is_active());
            crop_list.Add(crop);
        }
        stream.Add("crops", crop_list);
        stream_list.Add(stream);
    }
    outRootNode->Add("input_streams", stream_list);
}

void Mixer::get_stream(Jzon::Object* params, Jzon::Object* outRootNode)
{

    uint32_t id = params->Get("id").ToInt();

    Stream *str = layout->get_stream_by_id(id);

    if (str == NULL){
        outRootNode->Add("stream", Jzon::null);
        return;
    }

    Jzon::Array crop_list;
    std::map<uint32_t, Crop*>* crop_map;
    std::map<uint32_t, Crop*>::iterator crop_it;

    outRootNode->Add("id", (int)str->get_id());
    outRootNode->Add("width", (int)str->get_width());
    outRootNode->Add("height", (int)str->get_height());
    crop_map = str->get_crops();
    for (crop_it = crop_map->begin(); crop_it != crop_map->end(); crop_it++){
        Jzon::Object crop;
        crop.Add("id", (int)crop_it->second->get_id());
        crop.Add("c_w", (int)crop_it->second->get_crop_width());
        crop.Add("c_h", (int)crop_it->second->get_crop_height());
        crop.Add("c_x", (int)crop_it->second->get_crop_x());
        crop.Add("c_y", (int)crop_it->second->get_crop_y());
        crop.Add("dst_w", (int)crop_it->second->get_dst_width());
        crop.Add("dst_h", (int)crop_it->second->get_dst_height());
        crop.Add("dst_x", (int)crop_it->second->get_dst_x());
        crop.Add("dst_y", (int)crop_it->second->get_dst_y());
        crop.Add("layer", (int)crop_it->second->get_layer());
        crop.Add("state", (int)crop_it->second->is_active());
        crop_list.Add(crop);
    }
    outRootNode->Add("crops", crop_list);
}

void Mixer::get_crop_from_stream(Jzon::Object* params, Jzon::Object* outRootNode)
{

    uint32_t stream_id = params->Get("stream_id").ToInt();
    uint32_t crop_id = params->Get("crop_id").ToInt();

    Stream *stream = layout->get_stream_by_id(stream_id);

    if (stream == NULL){
        outRootNode->Add("stream", Jzon::null);
        return;
    }

    Crop* c = stream->get_crop_by_id(crop_id);

    if (c == NULL){
        outRootNode->Add("crop", Jzon::null);
        return;
    }

    outRootNode->Add("id", (int)c->get_id());
    outRootNode->Add("c_w", (int)c->get_crop_width());
    outRootNode->Add("c_h", (int)c->get_crop_height());
    outRootNode->Add("c_x", (int)c->get_crop_x());
    outRootNode->Add("c_y", (int)c->get_crop_y());
    outRootNode->Add("dst_w", (int)c->get_dst_width());
    outRootNode->Add("dst_h", (int)c->get_dst_height());
    outRootNode->Add("dst_x", (int)c->get_dst_x());
    outRootNode->Add("dst_y", (int)c->get_dst_y());
    outRootNode->Add("layer", (int)c->get_layer());
    outRootNode->Add("state", (int)c->is_active());
}

void Mixer::get_layout(Jzon::Object* params, Jzon::Object* outRootNode){

    Jzon::Array crop_list;
    std::map<uint32_t, Crop*>::iterator crop_it;
    std::map<uint32_t, Crop*> *crp = layout->get_out_stream()->get_crops();
    std::vector<Mixer::Dst>::iterator dst_it;
    std::vector<Mixer::Dst> dst;

    Jzon::Object stream;
    stream.Add("id", (int)layout->get_out_stream()->get_id());
    stream.Add("width", (int)layout->get_out_stream()->get_width());
    stream.Add("height", (int)layout->get_out_stream()->get_height());
    for (crop_it = crp->begin(); crop_it != crp->end(); crop_it++){
        Jzon::Object crop;
        Jzon::Array dst_list;
        crop.Add("id", (int)crop_it->second->get_id());
        crop.Add("c_w", (int)crop_it->second->get_crop_width());
        crop.Add("c_h", (int)crop_it->second->get_crop_height());
        crop.Add("c_x", (int)crop_it->second->get_crop_x());
        crop.Add("c_y", (int)crop_it->second->get_crop_y());
        crop.Add("dst_w", (int)crop_it->second->get_dst_width());
        crop.Add("dst_h", (int)crop_it->second->get_dst_height());

        dst = get_output_stream_destinations(crop_it->second->get_id());
        for (dst_it = dst.begin(); dst_it != dst.end(); dst_it++){
            Jzon::Object dst;
            dst.Add("id", (int)dst_it->id);
            dst.Add("ip", dst_it->ip);
            dst.Add("port", (int)dst_it->port);
            dst_list.Add(dst);
        }
        crop.Add("destinations", dst_list);
        crop_list.Add(crop);
    }
    stream.Add("crops", crop_list);
    outRootNode->Add("output_stream", stream);
}

void Mixer::get_layout_size(Jzon::Object* params, Jzon::Object* outRootNode)
{

    int width = layout->get_out_stream()->get_width();
    int height = layout->get_out_stream()->get_height();

    outRootNode->Add("width", width);
    outRootNode->Add("height", height);
}

void Mixer::get_stats(Jzon::Object* params, Jzon::Object* outRootNode)
{
#ifdef STATS
    map<uint32_t,streamStats*> input_stats; 
    map<uint32_t,streamStats*> output_stats; 
    s_mng->get_stats(input_stats, output_stats);
    map<uint32_t,streamStats*>::iterator str_it; 

    Jzon::Array input_list;
    for (str_it = input_stats.begin(); str_it != input_stats.end(); str_it++){
        Jzon::Object str;
        str.Add("id", (int)str_it->first);
        str.Add("delay", (int)str_it->second->get_delay());
        str.Add("fps", (int)str_it->second->get_fps());
        str.Add("bitrate", (int)str_it->second->get_bitrate());
        str.Add("lost_coded_frames", (int)str_it->second->get_lost_coded_frames());
        str.Add("lost_frames", (int)str_it->second->get_lost_frames());
        str.Add("total_frames", (int)str_it->second->get_total_frames());
        str.Add("lost_frames_percent", (int)str_it->second->get_lost_frames_percent());
        input_list.Add(str);
    }
    outRootNode->Add("input_streams", input_list);

    Jzon::Array output_list;
    for (str_it = output_stats.begin(); str_it != output_stats.end(); str_it++){
        Jzon::Object str;
        str.Add("id", (int)str_it->first);
        str.Add("delay", (int)str_it->second->get_delay());
        str.Add("fps", (int)str_it->second->get_fps());
        str.Add("bitrate", (int)str_it->second->get_bitrate());
        str.Add("lost_coded_frames", (int)str_it->second->get_lost_coded_frames());
        str.Add("lost_frames", (int)str_it->second->get_lost_frames());
        str.Add("total_frames", (int)str_it->second->get_total_frames());
        str.Add("lost_frames_percent", (int)str_it->second->get_lost_frames_percent());
        output_list.Add(str);
    }
    outRootNode->Add("output_streams", output_list);
#endif
}





