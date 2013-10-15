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
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

void* mixer::run(void) {
	int i, ret;
	struct participant_data* part;
	have_new_frame = false;
	should_stop = false;
	struct timeval start, finish;
	float diff = 0, min_diff = 0;
	pthread_mutex_init(&active_flag_mutex, NULL);

	min_diff = ((float)1/(float)max_frame_rate)*1000; // In ms

	while (!should_stop){
	
	    min_diff = ((float)1/(float)max_frame_rate)*1000;

		if (diff < min_diff){
			usleep((min_diff - diff)*1000); // We sleep a 10% of the minimum diff between loops
		}

		gettimeofday(&start, NULL);

		pthread_rwlock_rdlock(&src_p_list->lock);

		part = src_p_list->first;

		for (i=0; i<src_p_list->count; i++){
			pthread_mutex_lock(&part->lock);

			    if (part->new_frame == 1){
					if (layout->get_stream(part->id)->get_orig_w() == 0 && layout->get_stream(part->id)->get_orig_h() == 0 ){
						layout->update_stream(part->id, part->width, part->height);
						printf("Stream %u updated with part->width = %u and part->height = %u\n", part->id, part->width, part->height);
					}

				    layout->introduce_frame(part->id, (uint8_t*)part->frame, part->frame_length);
				    have_new_frame = true;
				    part->new_frame = 0;
			    }
			pthread_mutex_unlock(&part->lock);
			part = part->next;
		}

		pthread_rwlock_unlock(&src_p_list->lock);

		pthread_mutex_lock(&active_flag_mutex);
		if(set_active_flag == true){
			have_new_frame = true;
		}
		pthread_mutex_unlock(&active_flag_mutex);

		if (have_new_frame){
			layout->merge_frames();		
			pthread_rwlock_rdlock(&dst_p_list->lock);

			part = dst_p_list->first;

			for (i=0; i<dst_p_list->count; i++){
				if(set_active_flag==true){
					set_active_flag == false;
				}
				ret = pthread_mutex_lock(&part->lock);
				
				if(ret==0){
				    memcpy((uint8_t*)part->frame, (uint8_t*)layout->get_layout_bytestream(), layout->get_buffsize());
				    part->frame_length = layout->get_buffsize();
				    part->new_frame = 1;
				    pthread_mutex_unlock(&part->lock);
				}
				part = part->next;
			}

			pthread_rwlock_unlock(&dst_p_list->lock);
			
			have_new_frame = false;
		}
        
        
		gettimeofday(&finish, NULL);

		diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec)/1000; // In ms
	}

	stop_receiver(receiver);
	stop_out_manager();
	destroy_participant_list(dst_p_list);
	destinations.clear();
	delete layout;

}

void mixer::init(int layout_width, int layout_height, int max_streams, uint32_t in_port, uint32_t out_port){
	layout = new Layout(layout_width, layout_height, PIX_FMT_RGB24, max_streams);
	src_p_list = init_participant_list();
	dst_p_list = init_participant_list();
	receiver = init_receiver(src_p_list, in_port);
	_in_port = in_port;
	_out_port = out_port;
	dst_counter = 0;
	max_frame_rate = 20;
}

void mixer::exec(){
	start_receiver(receiver);
	start_out_manager(dst_p_list, 10);
	pthread_create(&thread, NULL, mixer::execute_run, this);
}

void mixer::stop(){
	should_stop = true;
}

int mixer::add_source(int new_w, int new_h, int x, int y, int layer, codec_t codec){
	int id = layout->introduce_stream(PIX_FMT_RGB24, new_w, new_h, x, y, PIX_FMT_RGB24, layer);
	if (id == -1){
		printf("You have reached the max number of simultaneous streams in the Mixer: %u\n", layout->get_max_streams());
		return -1;
	}
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

int mixer::modify_stream (int id, int width, int height, int x, int y, int layer, bool keep_aspect_ratio){
	if (layout == NULL)
		return -1;

	return layout->modify_stream(id, width, height, PIX_FMT_RGB24, x, y, layer, keep_aspect_ratio);
}

int mixer::resize_output (int width, int height, bool resize_streams){
	if (layout == NULL)
		return -1;

	return layout->modify_layout(width,height, PIX_FMT_RGB24, resize_streams);
}

void mixer::change_max_framerate(int frame_rate){
	max_frame_rate = frame_rate;
}

void mixer::get_stream_info(std::map<string, int> &str_map, int id){
	str_map["id"] = id;
	str_map["orig_width"] = layout->get_stream(id)->get_orig_w();
	str_map["orig_height"] = layout->get_stream(id)->get_orig_h();
	str_map["width"] = layout->get_stream(id)->get_curr_w();
	str_map["height"] = layout->get_stream(id)->get_curr_h();
	str_map["x"] = layout->get_stream(id)->get_x_pos();
	str_map["y"] = layout->get_stream(id)->get_y_pos();
	str_map["layer"] = layout->get_stream(id)->get_layer();
	str_map["active"] = layout->get_stream(id)->get_active();
}

std::vector<int> mixer::get_streams_id(){
	if (layout == NULL)
		return std::vector<int>();

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

int mixer::set_stream_active(int id, uint8_t active_flag){
	if (layout == NULL)
		return -1;

	pthread_rwlock_wrlock(&src_p_list->lock);
	set_active_participant(get_participant_id(src_p_list, id), active_flag);
	layout->set_active(id, active_flag);
	pthread_mutex_lock(&active_flag_mutex);
	set_active_flag = true;
	pthread_mutex_unlock(&active_flag_mutex);
	pthread_rwlock_unlock(&src_p_list->lock);
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

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[64];
  int  y;

  // Open file
  sprintf(szFilename, "/home/palau/TFG/layout_prints/layout%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;

  // Write header
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);

  // Write pixel data
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

  // Close file
  fclose(pFile);
}

void mixer::show_stream_info(){
	layout->print_active_stream_info();
}

uint8_t mixer::get_state(){
	return state;
}

void mixer::set_state(uint8_t s){
	state = s;
}
