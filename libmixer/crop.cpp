#include "crop.h"

Crop::Crop(uint32_t crop_id, uint32_t c_width, uint32_t c_height, uint32_t c_x, uint32_t c_y, uint32_t dst_layer, 
				uint32_t d_width, uint32_t d_height, uint32_t d_x, uint32_t d_y, Mat *stream_img_ref){

	id = crop_id;
	crop_width = c_width;
	crop_height = c_height;
	crop_x = c_x  ;
	crop_y = c_y ;
	layer = dst_layer;
	dst_x = d_x;
	dst_y = d_y;
	src_img = stream_img_ref;
	dst_img_size = Size(d_width, d_height);
}

int Crop::modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y){
	crop_width = new_crop_width;
	crop_height = new_crop_height;
	crop_x = new_crop_x;
	crop_y = new_crop_y;
}

int Crop::modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y){
	dst_img_size.width = new_dst_width;
	dst_img_size.height = new_dst_height;
	dst_x = new_dst_x;
	dst_y = new_dst_y;
}

int Crop::modify_layer(uint32_t new_layer){
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

	while (1) {

		//TODO:Wait for new frame

		//TODO: Protect Mat objects
        resize(img(Rect(crop_x, crop_y, crop_width, crop_height)), crop_img, crop_img.size(), 0, 0, INTER_LINEAR);

	}
}