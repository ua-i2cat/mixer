#include "crop.h"

Crop::Crop(uint32_t crop_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
        	uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y, Mat *stream_img_ref, 
        	 pthread_rwlock_t *str_lock_ref, pthread_cond_t *str_cond_ref, pthread_mutex_t *str_new_frame_lock_ref, uint8_t *str_new_frame_ref)
{

	id = crop_id;
	crop_x = c_x  ;
	crop_y = c_y ;
	layer = dst_layer;
	dst_x = d_x;
	dst_y = d_y;
	src_img = stream_img_ref;
	dst_img_size = Size(d_width, d_height);
	crop_img_size = Size(c_width, c_height);
	pthread_rwlock_init(&lock, NULL);
	stream_lock = str_lock_ref;
	new_frame_lock = str_new_frame_lock_ref;
	new_frame_cond = str_cond_ref;
	new_frame = str_new_frame_ref;

	//TODO:init thread
}

int Crop::modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y){
	crop_img_size.width = new_crop_width;
	crop_img_size.height = new_crop_height;
	crop_x = new_crop_x;
	crop_y = new_crop_y;
}

int Crop::modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y, uint32_t new_layer){
	dst_img_size.width = new_dst_width;
	dst_img_size.height = new_dst_height;
	dst_x = new_dst_x;
	dst_y = new_dst_y;
	layer = new_layer;
}

int Crop::enable_dst(){

}

int Crop::disable_dst(){

}

void* Crop::execute_resize(void *context){
	return ((Crop *)context)->resize();
}

void* Crop::resize(void){

	while (run) {

		//Check if the original frame is ready
		pthread_mutex_lock(new_frame_lock);
		while (!(*new_frame) {
		    pthread_cond_wait(new_frame_cond, new_frame_lock);
		}
		*new_frame = FALSE;
		pthread_mutex_unlock(new_frame_lock);

		pthread_rwlock_rdlock(stream_lock);
		pthread_rwlock_wrlock(lock);

        resize(*src_img(Rect(crop_x, crop_y, crop_width, crop_height)), crop_img, crop_img_sz, 0, 0, INTER_LINEAR);

		pthread_rwlock_unlock(lock);
		pthread_rwlock_unlock(stream_lock);

	}
}