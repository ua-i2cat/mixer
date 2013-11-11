#include <pthread.h>
#include "layout.h"
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

Layout::Layout(uint32_t width, uint32_t height)
{
	lay_width = width;
	lay_height = height;
	layout_img = Mat(height, width, CV_8UC3);
	out_stream = new Stream(rand(), width, height); 
	out_stream->add_crop(rand(), width, height, 0, 0, 0, width, height, 0, 0);
	pthread_rwlock_init(&layers_lock, NULL);
	pthread_rwlock_init(&streams_lock, NULL);
    pthread_rwlock_init(&layout_img_lock, NULL);
}

int Layout::add_stream(uint32_t stream_id, uint32_t width, uint32_t height){
	pthread_rwlock_wrlock(&streams_lock);
	if (streams.count(stream_id) > 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = new Stream(stream_id, width, height);
	uint32_t id = rand();
	Crop *crop = stream->add_crop(id, width, height, 0, 0, 0, width, height, 0, 0);
	streams[stream_id] = stream;
	pthread_rwlock_unlock(&streams_lock);

	pthread_rwlock_wrlock(&layers_lock);
	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));
	pthread_rwlock_unlock(&layers_lock);

	return TRUE;

}

Stream* Layout::get_stream_by_id(uint32_t stream_id){
	pthread_rwlock_rdlock(&streams_lock);
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return NULL;
	}

	Stream *stream = streams[stream_id];

	pthread_rwlock_unlock(&streams_lock);
	return stream;
}

int Layout::remove_stream(uint32_t stream_id){
	pthread_rwlock_wrlock(&streams_lock);
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = streams[stream_id];

	pthread_rwlock_wrlock(&layers_lock);

	for (map<uint32_t, Crop*>::iterator str_it = stream->get_crops().begin(); stream->get_crops().size() != 0; str_it++){
		multimap<uint32_t, Crop*>::iterator it = crops_by_layers.find(str_it->second->get_layer());  
		while (it->second->get_id() != str_it->second->get_id()){
			it++;
		}
		Crop *crop = it->second;
		crops_by_layers.erase(it);
		pthread_rwlock_wrlock(&layout_img_lock);
		layout_img(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
		pthread_rwlock_unlock(&layout_img_lock);
		pthread_rwlock_wrlock(stream->get_lock());
		stream->remove_crop(crop->get_id());
		pthread_rwlock_unlock(stream->get_lock());
	}

	streams.erase(stream_id);
	delete stream;

	pthread_rwlock_unlock(&layers_lock);
	pthread_rwlock_unlock(&streams_lock);
	return TRUE;
}

int Layout::add_crop_to_stream(uint32_t stream_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
					uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y){

	pthread_rwlock_rdlock(&streams_lock);
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	//TODO: check if introced values are valid

	Stream *stream = streams[stream_id];
	pthread_rwlock_wrlock(stream->get_lock());
	Crop *crop = stream->add_crop(rand(), crop_width, crop_height, crop_x, crop_y, layer, dst_width, dst_height, dst_x, dst_y);
	pthread_rwlock_unlock(stream->get_lock());
	pthread_rwlock_unlock(&streams_lock);

	pthread_rwlock_wrlock(&layers_lock);
	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));
	pthread_rwlock_unlock(&layers_lock);

	return TRUE;

}

int Layout::remove_crop_from_stream(uint32_t stream_id, uint32_t crop_id){
	pthread_rwlock_rdlock(&streams_lock);
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = streams[stream_id];

	pthread_rwlock_rdlock(stream->get_lock());
	Crop *crop = stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		pthread_rwlock_unlock(stream->get_lock());
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	pthread_rwlock_wrlock(&layers_lock);
	std::multimap<uint32_t, Crop*>::iterator it = crops_by_layers.find(crop->get_layer());  
	while (it->second->get_id() != crop->get_id()){
		it++;
	}
	crops_by_layers.erase(it);
	pthread_rwlock_unlock(&layers_lock);

	pthread_rwlock_wrlock(&layout_img_lock);
	layout_img(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	pthread_rwlock_unlock(&layout_img_lock);

	stream->remove_crop(crop_id);
	pthread_rwlock_unlock(stream->get_lock());
	pthread_rwlock_unlock(&streams_lock);
	
	return TRUE;
}

int Layout::modify_orig_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, 
									uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y){

	//TODO:check if values are valid
	pthread_rwlock_rdlock(&streams_lock);
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	Crop *crop = stream->get_crop_by_id(crop_id);
	pthread_rwlock_unlock(&streams_lock);

	if (crop == NULL){
		return FALSE;
	}

	crop->modify_crop(new_crop_width, new_crop_height, new_crop_x, new_crop_y);

	return TRUE;

}

int Layout::modify_dst_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y, uint32_t new_layer){

//TODO:check if values are valid
	pthread_rwlock_rdlock(&streams_lock);
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	Crop *crop = stream->get_crop_by_id(crop_id);
	pthread_rwlock_unlock(&streams_lock);

	if (crop == NULL){
		return FALSE;
	}

	pthread_rwlock_wrlock(&layout_img_lock);
	layout_img(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	crop->modify_dst(new_crop_width, new_crop_height, new_crop_x, new_crop_y, new_layer);
	pthread_rwlock_unlock(&layout_img_lock);

	return TRUE;

}

void Layout::compose_layout()
{
	pthread_rwlock_rdlock(&layers_lock);
	pthread_rwlock_wrlock(&layout_img_lock);
	for (multimap<uint32_t, Crop*>::iterator it = crops_by_layers.begin(); it != crops_by_layers.end(); it++){
		Crop *crop = it->second;
		if (crop->get_crop_img().cols != 0 && crop->get_crop_img().rows != 0){
			pthread_rwlock_rdlock(crop->get_lock());
			crop->get_crop_img().copyTo(layout_img(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())));
			pthread_rwlock_unlock(crop->get_lock());
		}
	}
	pthread_rwlock_unlock(&layout_img_lock);
	pthread_rwlock_unlock(&layers_lock);
}

Stream* Layout::get_out_stream()
{
	return out_stream;
}

int Layout::introduce_frame_to_stream(uint32_t stream_id, uint8_t* buffer, uint32_t buffer_length)
{
	Stream *stream = get_stream_by_id(stream_id);

	if (stream == NULL){
		return FALSE;
	}

	pthread_rwlock_wrlock(stream->get_lock());
	int ret = stream->introduce_frame(buffer, buffer_length);
	pthread_rwlock_unlock(stream->get_lock());

	return ret;
}

int enable_crop_from_stream(uint32_t stream_id, uint32_t crop_id)
{
	
}
int disable_crop_from_stream(uint32_t stream_id, uint32_t crop_id);

uint8_t* Layout::get_buffer()
{
	pthread_rwlock_rdlock(&layout_img_lock);
	uint8_t* out_buffer = (uint8_t*)layout_img.data;
	pthread_rwlock_unlock(&layout_img_lock);
	return out_buffer;
}

uint32_t Layout::get_buffer_size()
{
	return (layout_img.step * lay_height * sizeof(uint8_t));
}