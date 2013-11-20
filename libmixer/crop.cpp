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

Crop::Crop(uint32_t crop_id, uint32_t c_width, uint32_t c_height, uint32_t c_x, uint32_t c_y, 
        	uint32_t dst_layer, uint32_t d_width, uint32_t d_height, uint32_t d_x, uint32_t d_y, Mat stream_img_ref, 
        	 pthread_rwlock_t *str_lock_ref)
{
	id = crop_id;
	crop_x = c_x ;
	crop_y = c_y;
	layer = dst_layer;
	dst_x = d_x;
	dst_y = d_y;
	dst_img_size = Size(d_width, d_height);
	crop_img_size = Size(c_width, c_height);
	src_cropped_img = stream_img_ref(Rect(c_x, c_y, c_width, c_height));
	pthread_rwlock_init(&lock, NULL);
	stream_lock = str_lock_ref;
	new_frame = FALSE;
	run = TRUE;
	active = TRUE;

	pthread_create(&thread, NULL, Crop::execute_resize, this);
}

void Crop::modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y, Mat stream_img_ref)
{
	crop_img_size.width = new_crop_width;
	crop_img_size.height = new_crop_height;
	crop_x = new_crop_x;
	crop_y = new_crop_y;
	src_cropped_img = stream_img_ref(Rect(new_crop_x, new_crop_y, new_crop_width, new_crop_height));
}

void Crop::modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y, uint32_t new_layer)
{
	dst_img_size.width = new_dst_width;
	dst_img_size.height = new_dst_height;
	dst_x = new_dst_x;
	dst_y = new_dst_y;
	layer = new_layer;
	crop_img.create(new_dst_height, new_dst_width, CV_8UC3);
}

void* Crop::execute_resize(void *context)
{
	return ((Crop *)context)->resize_routine();
}

void* Crop::resize_routine(void)
{
	 while (run) {
		usleep(100);//TODO: set a non magic number in this usleep (maybe related to framerate)
                
        if (new_frame == FALSE){
            continue;
        }
        
        if (!run) {
            break;
        }

        new_frame = FALSE;

		pthread_rwlock_rdlock(stream_lock);
		pthread_rwlock_wrlock(&lock);
        
        resize(src_cropped_img, crop_img, dst_img_size, 0, 0, INTER_LINEAR);
		
		pthread_rwlock_unlock(&lock);
		pthread_rwlock_unlock(stream_lock);
	 }
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
	return crop_img;
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
	return dst_img_size.width;
}

uint32_t Crop::get_dst_height(){
	return dst_img_size.height;
}

pthread_rwlock_t* Crop::get_lock()
{
	return &lock;
}

pthread_t Crop::get_thread()
{
	return thread;
} 

void Crop::stop(){
	run = FALSE;

	new_frame = TRUE;
}

uint8_t Crop::is_active()
{
	return active;
}

uint8_t Crop::set_active(uint8_t act)
{
	active = act;
}

uint8_t* Crop::get_buffer()
{
	return crop_img.data;
}

uint32_t Crop::get_buffer_size()
{
	return (crop_img.step * dst_img_size.height * sizeof(uint8_t));
}

void Crop::set_new_frame(uint8_t n)
{
	new_frame = n;
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

