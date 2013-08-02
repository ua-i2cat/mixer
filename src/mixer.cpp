/*
 * mixer.cpp
 *
 *  Created on: Jul 17, 2013
 *      Author: palau
 */

#include <iostream>
#include "mixer.h"
#include <sys/time.h>
extern "C"{
	#include "transmitter.h"
}

using namespace std;

mixer* mixer::mixer_instance;
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);

void* mixer::run(void) {
	int i;
	struct participant_data* part;
	bool have_new_frame = false;
	should_stop = false;
	struct timeval start, finish;
	float diff = 0, min_diff = 0;
	int frame_cont = 0, save_cont=5;

	min_diff = ((float)1/(float)max_frame_rate)*1000; // In ms

	while (!should_stop){
	
	    min_diff = ((float)1/(float)max_frame_rate)*1000;

		if (diff < min_diff){
			usleep((min_diff - diff)*1000); // We sleep a 10% of the minimum diff between loops
			printf("(min_diff - diff): %f  diff: %f   min_diff: %f\n", min_diff - diff, diff, min_diff);
		}

		gettimeofday(&start, NULL);

		pthread_rwlock_rdlock(&src_p_list->lock);

		part = src_p_list->first;

		for (i=0; i<src_p_list->count; i++){
			if(pthread_mutex_trylock(&part->lock)==0){
			    if (part->new_frame == TRUE){
				    layout.introduce_frame(part->id, (uint8_t*)part->frame, part->frame_length);
				    have_new_frame = true;
				    part->new_frame = FALSE;
			    }
			    pthread_mutex_unlock(&part->lock);
			}
			part = part->next;
		}

		pthread_rwlock_unlock(&src_p_list->lock);

		if (have_new_frame){
			layout.merge_frames();
//			if(save_cont-- == 0){
//			    SaveFrame(layout.get_lay_frame(), layout.get_w(), layout.get_h(), frame_cont);
//			    save_cont = 10;
//			}
//			frame_cont++;
			
			pthread_rwlock_rdlock(&dst_p_list->lock);

			part = dst_p_list->first;

			for (i=0; i<dst_p_list->count; i++){
				if(pthread_mutex_trylock(&part->lock)==0){
				memcpy((uint8_t*)part->frame, (uint8_t*)layout.get_layout_bytestream(), layout.get_buffsize());
				part->frame_length = layout.get_buffsize();
				part->new_frame = 1;
				pthread_mutex_unlock(&part->lock);
				}
				part = part->next;
			}

			pthread_rwlock_unlock(&dst_p_list->lock);
			
			notify_out_manager();
		}
        
		have_new_frame = false;
        
		gettimeofday(&finish, NULL);

		diff = ((finish.tv_sec - start.tv_sec)*1000000 + finish.tv_usec - start.tv_usec)/1000; // In ms
	}
}

void mixer::init(int layout_width, int layout_height, int max_streams, uint32_t in_port, uint32_t out_port){
	layout.init(layout_width, layout_height, PIX_FMT_RGB24, max_streams);
	src_p_list = init_participant_list();
	dst_p_list = init_participant_list();
	reciever = init_reciever(src_p_list, in_port);
	_in_port = in_port;
	_out_port = out_port;
	dst_counter = 0;
	max_frame_rate = 30;
}

void mixer::exec(){
	start_reciever(reciever);
	start_out_manager(dst_p_list);
	pthread_create(&thread, NULL, mixer::execute_run, this);
}

void mixer::stop(){
	should_stop = true;
}

int mixer::add_source(uint32_t width, uint32_t height, codec_t codec){
	int id = layout.introduce_stream(width, height, PIX_FMT_RGB24, width, height, 0, 0, PIX_FMT_RGB24, 0);
	if (id == -1){
		printf("You have reached the max number of simultaneous streams in the Mixer: %u\n", layout.get_max_streams());
		return -1;
	}
	pthread_rwlock_wrlock(&src_p_list->lock);
	int ret = add_participant(src_p_list, id, width, height, codec, NULL, 0, INPUT);
	pthread_rwlock_unlock(&src_p_list->lock);
	return ret;
}

int mixer::remove_source(uint32_t id){
	layout.remove_stream(id);
	printf("Just after removing stream\n");
	pthread_rwlock_wrlock(&src_p_list->lock);
	printf("Just before removing participant\n");
	int ret = remove_participant(src_p_list, id);
	printf("Just after removing participant\n");
	pthread_rwlock_unlock(&src_p_list->lock);
	return ret;
}

int mixer::add_destination(codec_t codec, char *ip, uint32_t port){
	pthread_rwlock_wrlock(&dst_p_list->lock);
	int ret =  add_participant(dst_p_list, dst_counter++, layout.get_w(), layout.get_h(), codec, ip, port, OUTPUT);
	pthread_rwlock_unlock(&dst_p_list->lock);
	return ret;
}

int mixer::remove_destination(uint32_t id){
	pthread_rwlock_wrlock(&dst_p_list->lock);
	int ret = remove_participant(dst_p_list, id);
	pthread_rwlock_unlock(&dst_p_list->lock);
	return ret;
}

int mixer::modify_stream (int id, int width, int height, int x, int y, int layer, bool keep_aspect_ratio){
	return layout.modify_stream(id, width, height, PIX_FMT_RGB24, x, y, layer, keep_aspect_ratio);
}

int mixer::resize_output (int width, int height, bool resize_streams){
	return layout.modify_layout(width,height, PIX_FMT_RGB24, resize_streams);
}

void mixer::change_max_framerate(int frame_rate){
	max_frame_rate = frame_rate;
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
  sprintf(szFilename, "./layout%d.ppm", iFrame);
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
