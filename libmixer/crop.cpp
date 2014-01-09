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

#include "crop.h"
#include <stdio.h>
#include <iostream>

Crop::Crop(uint32_t crop_id, uint32_t c_width, uint32_t c_height, uint32_t c_x, uint32_t c_y, 
        	uint32_t dst_layer, uint32_t d_width, uint32_t d_height, uint32_t d_x, uint32_t d_y, Mat& stream_img_ref):src_img(stream_img_ref)
{
	id = crop_id;
	crop_x = c_x ;
	crop_y = c_y;
	layer = dst_layer;
	dst_x = d_x;
	dst_y = d_y;
	rsz_img_size = Size(d_width, d_height);
	crop_img_size = Size(c_width, c_height);
	src_img = stream_img_ref;
	rect_crop = Rect(c_x, c_y, c_width, c_height);
	active = TRUE;
	opacity = 1;
}

Crop::Crop(uint32_t crop_id, Mat& stream_img_ref):src_img(stream_img_ref)
{
	id = crop_id;
	crop_x = 0;
	crop_y = 0;
	crop_img_size.width = 0;
	crop_img_size.height = 0;
	rsz_img_size.width = 0;
	rsz_img_size.height = 0;
	layer = 0;
	dst_x = 0;
	dst_y = 0;
	opacity = 1;
	active = FALSE;
	src_img = stream_img_ref;
}

void Crop::init_input_values(uint32_t width, uint32_t height, uint32_t x, uint32_t y, Mat& stream_img_ref)
{
	crop_img_size.width = width;
	crop_img_size.height = height;
	crop_x = x;
	crop_y = y;
	src_img = stream_img_ref;
	rect_crop = Rect(x, y, width, height);
}

void Crop::modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y)
{
	crop_img_size.width = new_crop_width;
	crop_img_size.height = new_crop_height;
	crop_x = new_crop_x;
	crop_y = new_crop_y;
	rect_crop = Rect(new_crop_x, new_crop_y, new_crop_width, new_crop_height);
}

void Crop::modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y, uint32_t new_layer, double op)
{
	rsz_img_size.width = new_dst_width;
	rsz_img_size.height = new_dst_height;
	dst_x = new_dst_x;
	dst_y = new_dst_y;
	layer = new_layer;
	resized_img.create(new_dst_height, new_dst_width, CV_8UC3);
	opacity = op;
}

void* Crop::execute_resize(void *context)
{
	return ((Crop *)context)->resize_routine();
}

void* Crop::resize_routine(void)
{ 
	if (rect_crop.width == rsz_img_size.width && rect_crop.height == rsz_img_size.height){
		src_img(rect_crop).copyTo(resized_img);
		return 0;
	}

    resize(src_img(rect_crop), resized_img, rsz_img_size, 0, 0, INTER_LINEAR);
}

uint32_t Crop::get_layer()
{
	return layer;
}

uint32_t Crop::get_id()
{
	return id;
}

Mat Crop::get_crop_img()
{
	return resized_img;
}

uint32_t Crop::get_dst_x()
{
	return dst_x;
}

uint32_t Crop::get_dst_y()
{
	return dst_y;
}

uint32_t Crop::get_dst_width(){
	return rsz_img_size.width;
}

uint32_t Crop::get_dst_height(){
	return rsz_img_size.height;
}

pthread_rwlock_t* Crop::get_lock()
{
	return &lock;
}

pthread_t* Crop::get_thread()
{
	return &thread;
} 

uint8_t Crop::is_active()
{
	return active;
}

void Crop::set_active(uint8_t act)
{
	active = act;
}

uint8_t* Crop::get_buffer()
{
	return resized_img.data;
}

uint32_t Crop::get_buffer_size()
{
	return (resized_img.step * rsz_img_size.height * sizeof(uint8_t));
}

uint32_t Crop::get_crop_width()
{
	return crop_img_size.width;
}
		
uint32_t Crop::get_crop_height()
{
	return crop_img_size.height;
}

uint32_t Crop::get_crop_x()
{
	return crop_x;
}

uint32_t Crop::get_crop_y()
{
	return crop_y;
}

void Crop::set_resized_buffer(uint8_t* buffer){
	resized_img.data = buffer;
}

double Crop::get_opacity()
{
	return opacity;
}



