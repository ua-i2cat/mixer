/*
 *  LIBMIXER - A video frame mixing library
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin LIBMIXER.
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
 */

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
	layers_it = crops_by_layers.begin();
}

int Layout::add_stream(uint32_t stream_id, uint32_t width, uint32_t height){
	if (streams.count(stream_id) > 0) {
		return FALSE;
	}

	Stream *stream = new Stream(stream_id, width, height);
	uint32_t id = rand();
	Crop *crop = stream->add_crop(id, width, height, 0, 0, 0, out_stream->get_width(), out_stream->get_height(), 0, 0);
	streams[stream_id] = stream;

	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));

	return TRUE;

}

int Layout::add_stream(uint32_t stream_id)
{
	if (streams.count(stream_id) > 0) {
		return FALSE;
	}

	Stream *stream = new Stream(stream_id);
	streams[stream_id] = stream;

	return TRUE;
}

int Layout::init_stream(uint32_t stream_id, uint32_t width, uint32_t height)
{
	if (streams.count(stream_id) <= 0) {
		return FALSE;
	}

	streams[stream_id]->init(width, height);

	uint32_t id = rand();
	Crop *crop = streams[stream_id]->add_crop(id, width, height, 0, 0, 0, out_stream->get_width(), out_stream->get_height(), 0, 0);

	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));

	return TRUE;
}

Stream* Layout::get_stream_by_id(uint32_t stream_id)
{
	if (streams.count(stream_id) <= 0) {
		return NULL;
	}

	Stream *stream = streams[stream_id];

	return stream;
}

int Layout::remove_stream(uint32_t stream_id)
{
	if (streams.count(stream_id) <= 0) {
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	map<uint32_t, Crop*> s_map = stream->get_crops();
	map<uint32_t, Crop*>::iterator str_it;

	for (str_it = s_map.begin(); str_it != s_map.end(); str_it++){
		multimap<uint32_t, Crop*>::iterator it = crops_by_layers.find(str_it->second->get_layer());  
		while (it->second->get_id() != str_it->second->get_id()){
			it++;
		}
		Crop *crop = it->second;
		crops_by_layers.erase(it);
		out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	}

	streams.erase(stream_id);
	delete stream;

	return TRUE;
}

int Layout::add_crop_to_stream(uint32_t stream_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
					uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y)
{

	if (streams.count(stream_id) <= 0) {
		return FALSE;
	}

	if (!check_values(out_stream->get_width(), out_stream->get_height(), dst_width, dst_height, dst_x, dst_y)){
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	if (!check_values(stream->get_width(), stream->get_height(), crop_width, crop_height, crop_x, crop_y)){
		return FALSE;
	}

	Crop *crop = stream->add_crop(rand(), crop_width, crop_height, crop_x, crop_y, layer, dst_width, dst_height, dst_x, dst_y);
	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));

	return TRUE;

}

int Layout::remove_crop_from_stream(uint32_t stream_id, uint32_t crop_id)
{

	if (streams.count(stream_id) <= 0) {
		return FALSE;
	}

	Stream *stream = streams[stream_id];

	Crop *crop = stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	std::multimap<uint32_t, Crop*>::iterator it = crops_by_layers.find(crop->get_layer());  
	while (it->second->get_id() != crop->get_id()){
		it++;
	}
	crops_by_layers.erase(it);

	out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);

	stream->remove_crop(crop_id);
	
	return TRUE;
}

int Layout::modify_orig_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, 
									uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y){

	if (streams.count(stream_id) <= 0) {
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	Crop *crop = stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	if (!check_values(stream->get_width(), stream->get_height(), new_crop_width, new_crop_height, new_crop_x, new_crop_y)){
		return FALSE;
	}

	crop->modify_crop(new_crop_width, new_crop_height, new_crop_x, new_crop_y, stream->get_img());

	return TRUE;

}

int Layout::modify_dst_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y, uint32_t new_layer){

	if (streams.count(stream_id) <= 0) {
		return FALSE;
	}

	Stream *stream = streams[stream_id];
	Crop *crop = stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	if (!check_values(out_stream->get_width(), out_stream->get_height(), new_crop_width, new_crop_height, new_crop_x, new_crop_y)){
		return FALSE;
	}

	out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);
	
	layers_it = crops_by_layers.find(crop->get_layer());  
	while (layers_it->second->get_id() != crop->get_id()){
		layers_it++;
	}
	crops_by_layers.erase(layers_it);
	crop->modify_dst(new_crop_width, new_crop_height, new_crop_x, new_crop_y, new_layer);
	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));

	return TRUE;

}

void Layout::compose_layout()
{
	for (layers_it = crops_by_layers.begin(); layers_it != crops_by_layers.end(); layers_it++){
		Crop *crop = layers_it->second;
		crop->get_crop_img().copyTo(out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())));
	}
}

Stream* Layout::get_out_stream()
{
	return out_stream;
}

void Layout::introduce_frame_to_stream(uint32_t stream_id, uint8_t* buffer, uint32_t buffer_length)
{
	streams[stream_id]->introduce_frame(buffer, buffer_length);
}

void Layout::resize_input_crops()
{
	for (streams_it = streams.begin(); streams_it != streams.end(); streams_it++){
		if (!streams_it->second->has_new_frame()){
			continue;
		}
		streams_it->second->create_resize_crops_routine();
	}

	for (streams_it = streams.begin(); streams_it != streams.end(); streams_it++){
		if (!streams_it->second->has_new_frame()){
			continue;
		}
		streams_it->second->wait_for_resize_crops_routine();
		streams_it->second->set_new_frame(FALSE);
	}
}

void Layout::split_layout()
{
	out_stream->resize_crops();
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

	crop->set_active(TRUE);

	crops_by_layers.insert(pair<uint32_t, Crop*>(crop->get_layer(), crop));

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

	crop->set_active(FALSE);

	std::multimap<uint32_t, Crop*>::iterator it = crops_by_layers.find(crop->get_layer());  
	while (it->second->get_id() != crop->get_id()){
		it++;
	}
	crops_by_layers.erase(it);

	out_stream->get_img()(Rect(crop->get_dst_x(), crop->get_dst_y(), crop->get_dst_width(), crop->get_dst_height())) = Mat::zeros(crop->get_dst_height(), crop->get_dst_width(), CV_8UC3);

	return TRUE;
}

int Layout::add_crop_to_output_stream(uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t rsz_width, uint32_t rsz_height)
{
	if (!check_values(out_stream->get_width(), out_stream->get_height(), crop_width, crop_height, crop_x, crop_y)){
		return FALSE;
	}

	//TODO: check if dst_widht and rsz_height are bigger thant MAX SIZE ?
	uint32_t crop_id = rand();

	Crop *crop = out_stream->add_crop(crop_id, crop_width, crop_height, crop_x, crop_y, 0, rsz_width, rsz_height, 0, 0);

	if (crop == NULL){
		return FALSE;
	}

	return crop_id;
}

int Layout::modify_crop_from_output_stream(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	if (!check_values(out_stream->get_width(), out_stream->get_height(), new_crop_width, new_crop_height, new_crop_x, new_crop_y)){
		return FALSE;
	}

	crop->modify_crop(new_crop_width, new_crop_height, new_crop_x, new_crop_y, out_stream->get_img());

	return TRUE;
}

int Layout::modify_crop_resize_from_output_stream(uint32_t crop_id, uint32_t new_width, uint32_t new_height)
{
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	//TODO;; check if width and height are bigger thant MAX SIZE;

	crop->modify_dst(new_width, new_height, 0, 0, 0);

	return TRUE;
}

int Layout::remove_crop_from_output_stream(uint32_t crop_id)
{
	uint8_t ret = out_stream->remove_crop(crop_id);

	return ret;
}

uint8_t* Layout::get_buffer()
{
	uint8_t* out_buffer = (uint8_t*)out_stream->get_img().data;
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

uint8_t Layout::check_if_stream_init(uint32_t stream_id)
{
	if (streams[stream_id]->get_width() == 0 && streams[stream_id]->get_height() == 0){
		return FALSE;
	}

	return TRUE;
}

uint8_t* Layout::get_output_crop_buffer(uint32_t crop_id)
{
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	uint8_t* buffer = crop->get_buffer();

	return buffer;
}
        
uint32_t Layout::get_output_crop_buffer_size(uint32_t crop_id)
{
	Crop *crop = out_stream->get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	uint32_t buffer_size = crop->get_buffer_size();

	return buffer_size;
}

map<uint32_t, Stream*> Layout::get_streams()
{
	return streams;
}

int Layout::set_resized_output_buffer(uint32_t id, uint8_t *buffer)
{
	Crop *crop = out_stream->get_crop_by_id(id);

	if (crop == NULL){
		return FALSE;
	}

	crop->set_resized_buffer(buffer);
	
	return TRUE;
}
