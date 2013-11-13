#include "crop.h"
#include <stdio.h>

Crop::Crop(uint32_t crop_id, uint32_t c_width, uint32_t c_height, uint32_t c_x, uint32_t c_y, 
        	uint32_t dst_layer, uint32_t d_width, uint32_t d_height, uint32_t d_x, uint32_t d_y, Mat stream_img_ref, 
        	 pthread_rwlock_t *str_lock_ref, pthread_cond_t *str_cond_ref, pthread_mutex_t *str_new_frame_lock_ref, uint8_t *str_new_frame_ref)
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
	new_frame_lock = str_new_frame_lock_ref;
	new_frame_cond = str_cond_ref;
	new_frame = str_new_frame_ref;
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
}

void* Crop::execute_resize(void *context)
{
	return ((Crop *)context)->resize_routine();
}

void* Crop::resize_routine(void)
{
	while (run) {
		//Check if the original frame is ready
		pthread_mutex_lock(new_frame_lock);
		while (!(*new_frame)) {
		    pthread_cond_wait(new_frame_cond, new_frame_lock);
		}

		*new_frame = FALSE;
		pthread_mutex_unlock(new_frame_lock);

		if (!run){
			break;
		}

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

	pthread_mutex_lock(new_frame_lock);
	*new_frame = TRUE;
	pthread_cond_signal(new_frame_cond);
	pthread_mutex_unlock(new_frame_lock);
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

