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
	out_stream = new Stream(rand(), width, height); 
	//out_stream->add_crop(rand(), width, height, 0, 0, 0, width, height, 0, 0);
	pthread_rwlock_init(&layers_lock, NULL);
	pthread_rwlock_init(&streams_lock, NULL);
	layers_it = crops_by_layers.begin();
}

int Layout::add_stream(uint32_t stream_id, uint32_t width, uint32_t height){
	pthread_rwlock_wrlock(&streams_lock);
	if (streams.count(stream_id) > 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = new Stream(stream_id, width, height);
	uint32_t id = rand();
	Crop *crop = stream->add_crop(id, width, height, 0, 0, 0, out_stream->get_width(), out_stream->get_height(), 0, 0);
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
		pthread_rwlock_wrlock(out_stream->get_lock());
		out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
		pthread_rwlock_unlock(out_stream->get_lock());
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

	if (!check_values(out_stream->get_width(), out_stream->get_height(), dst_width, dst_height, dst_x, dst_y)){
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	pthread_rwlock_wrlock(stream->get_lock());
	if (!check_values(stream->get_width(), stream->get_height(), crop_width, crop_height, crop_x, crop_y)){
		pthread_rwlock_unlock(stream->get_lock());
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

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

	pthread_rwlock_wrlock(out_stream->get_lock());
	out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	pthread_rwlock_unlock(out_stream->get_lock());

	stream->remove_crop(crop_id);
	pthread_rwlock_unlock(stream->get_lock());
	pthread_rwlock_unlock(&streams_lock);
	
	return TRUE;
}

int Layout::modify_orig_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, 
									uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y){

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

	pthread_rwlock_rdlock(stream->get_lock());
	if (!check_values(stream->get_width(), stream->get_height(), new_crop_width, new_crop_height, new_crop_x, new_crop_y)){
		pthread_rwlock_unlock(stream->get_lock());
		return FALSE;
	}

	crop->modify_crop(new_crop_width, new_crop_height, new_crop_x, new_crop_y, stream->get_img());
	pthread_rwlock_unlock(stream->get_lock());

	return TRUE;

}

int Layout::modify_dst_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y, uint32_t new_layer){

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

	if (!check_values(out_stream->get_width(), out_stream->get_height(), new_crop_width, new_crop_height, new_crop_x, new_crop_y)){
		return FALSE;
	}

	pthread_rwlock_wrlock(out_stream->get_lock());
	out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	crop->modify_dst(new_crop_width, new_crop_height, new_crop_x, new_crop_y, new_layer);
	pthread_rwlock_unlock(out_stream->get_lock());

	return TRUE;

}

void Layout::compose_layout()
{
	pthread_rwlock_rdlock(&layers_lock);
	pthread_rwlock_wrlock(out_stream->get_lock());
	for (layers_it = crops_by_layers.begin(); layers_it != crops_by_layers.end(); layers_it++){
		Crop *crop = layers_it->second;
		if (crop->get_crop_img().cols == 0 && crop->get_crop_img().rows == 0){
			continue;
		}
		
		pthread_rwlock_rdlock(crop->get_lock());
		crop->get_crop_img().copyTo(out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())));
		pthread_rwlock_unlock(crop->get_lock());
	}
	pthread_rwlock_unlock(out_stream->get_lock());
	pthread_rwlock_unlock(&layers_lock);

	out_stream->wake_up_crops();
}

Stream* Layout::get_out_stream()
{
	return out_stream;
}

int Layout::introduce_frame_to_stream(uint32_t stream_id, uint8_t* buffer, uint32_t buffer_length)
{
	pthread_rwlock_wrlock(streams[stream_id]->get_lock());
	int ret = streams[stream_id]->introduce_frame(buffer, buffer_length);
	pthread_rwlock_unlock(streams[stream_id]->get_lock());

	return ret;
}

int Layout::enable_crop_from_stream(uint32_t stream_id, uint32_t crop_id)
{
	Stream *stream = get_stream_by_id(stream_id);

	if (stream == NULL){
		return FALSE;
	}

	Crop *crop = stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	if (crop->is_active()){
		return FALSE;
	}

	pthread_rwlock_wrlock(crop->get_lock());
	crop->set_active(TRUE);
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_rdlock(crop->get_lock());
	pthread_rwlock_wrlock(&layers_lock);
	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));
	pthread_rwlock_unlock(&layers_lock);
	pthread_rwlock_unlock(crop->get_lock());

	return TRUE;
}
int Layout::disable_crop_from_stream(uint32_t stream_id, uint32_t crop_id)
{
	Stream *stream = get_stream_by_id(stream_id);

	if (stream == NULL){
		return FALSE;
	}

	Crop *crop = stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	if (!crop->is_active()){
		return FALSE;
	}

	pthread_rwlock_wrlock(crop->get_lock());
	crop->set_active(FALSE);
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_rdlock(crop->get_lock());
	pthread_rwlock_wrlock(&layers_lock);
	std::multimap<uint32_t, Crop*>::iterator it = crops_by_layers.find(crop->get_layer());  
	while (it->second->get_id() != crop->get_id()){
		it++;
	}
	crops_by_layers.erase(it);
	pthread_rwlock_unlock(&layers_lock);
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_wrlock(out_stream->get_lock());
	out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	pthread_rwlock_unlock(out_stream->get_lock());

	return TRUE;
}

uint32_t Layout::add_crop_to_output_stream(uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t dst_width, uint32_t dst_height)
{
	pthread_rwlock_wrlock(out_stream->get_lock());
	if (!check_values(out_stream->get_width(), out_stream->get_height(), crop_width, crop_height, crop_x, crop_y)){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	//TODO: check if dst_widht and dsT_height are bigger thant MAX SIZE ?
	uint32_t crop_id = rand();

	Crop *crop = out_stream->add_crop(crop_id, crop_width, crop_height, crop_x, crop_y, 0, dst_width, dst_height, 0, 0);

	if (crop == NULL){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	pthread_rwlock_unlock(out_stream->get_lock());
	return crop_id;
}

int Layout::modify_crop_from_output_stream(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	pthread_rwlock_rdlock(out_stream->get_lock());
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	if (!check_values(out_stream->get_width(), out_stream->get_height(), new_crop_width, new_crop_height, new_crop_x, new_crop_y)){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	pthread_rwlock_wrlock(crop->get_lock());
	crop->modify_crop(new_crop_width, new_crop_height, new_crop_x, new_crop_y, out_stream->get_img());
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_unlock(out_stream->get_lock());
	return TRUE;
}

int Layout::modify_crop_resize_from_output_stream(uint32_t crop_id, uint32_t new_width, uint32_t new_height)
{
	pthread_rwlock_rdlock(out_stream->get_lock());
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	//TODO;; check if width and height are bigger thant MAX SIZE;

	pthread_rwlock_wrlock(crop->get_lock());
	crop->modify_dst(new_width, new_height, 0, 0, 0);
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_unlock(out_stream->get_lock());
	return TRUE;
}

int Layout::remove_crop_from_output_stream(uint32_t crop_id)
{
	pthread_rwlock_rdlock(out_stream->get_lock());
	uint8_t ret = out_stream->remove_crop(crop_id);
	pthread_rwlock_unlock(out_stream->get_lock());

	return ret;
}

uint8_t* Layout::get_buffer()
{
	pthread_rwlock_rdlock(out_stream->get_lock());
	uint8_t* out_buffer = (uint8_t*)out_stream->get_img().data;
	pthread_rwlock_unlock(out_stream->get_lock());
	return out_buffer;
}

uint32_t Layout::get_buffer_size()
{
	return (out_stream->get_img().step * out_stream->get_height() * sizeof(uint8_t));
}

uint8_t Layout::check_values(uint32_t max_width, uint32_t max_height, uint32_t width, uint32_t height, uint32_t x, uint32_t y)
{
	if (x < 0 || y <0 || width <= 0 || height <= 0){
		return FALSE;
	}

	if ((width + x > max_width) || (height + y > max_height)){
		return FALSE;
	}

	return TRUE;
}

uint8_t Layout::check_if_stream(uint32_t stream_id)
{
	pthread_rwlock_rdlock(&streams_lock);
	
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&streams_lock);
		return FALSE;
	}

	pthread_rwlock_unlock(&streams_lock);
	return TRUE;
}

uint8_t* Layout::get_output_crop_buffer(uint32_t crop_id)
{
	pthread_rwlock_rdlock(out_stream->get_lock());
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	pthread_rwlock_rdlock(crop->get_lock());
	uint8_t* buffer = crop->get_buffer();
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_unlock(out_stream->get_lock());

	return buffer;
}
        
uint32_t Layout::get_output_crop_buffer_size(uint32_t crop_id)
{
	pthread_rwlock_rdlock(out_stream->get_lock());
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		pthread_rwlock_unlock(out_stream->get_lock());
		return FALSE;
	}

	pthread_rwlock_rdlock(crop->get_lock());
	uint32_t buffer_size = crop->get_buffer_size();
	pthread_rwlock_unlock(crop->get_lock());

	pthread_rwlock_unlock(out_stream->get_lock());
	return buffer_size;
}

map<uint32_t, Stream*> Layout::get_streams()
{
	return streams;
}

pthread_rwlock_t* Layout::get_streams_lock()
{
	return &streams_lock;
}
