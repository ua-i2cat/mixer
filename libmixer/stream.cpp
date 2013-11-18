#include <pthread.h>
#include "stream.h"
#include <iostream>
#include <stdio.h>
#include <sys/time.h>

using namespace std;
using namespace cv;

Stream::Stream(uint32_t stream_id, uint32_t stream_width, uint32_t stream_height)
{
	id = stream_id;
	sz = Size(stream_width, stream_height);
	img = Mat(stream_height, stream_width, CV_8UC3); //NOTE: height and width are correctly placed in the constructor-> Mat(rows, cols)
	it = crops.begin();
	pthread_rwlock_init(&lock, NULL);
	pthread_rwlock_init(&crops_lock, NULL);
} 

Crop* Stream::add_crop(uint32_t id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y,
				uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y)
{
	pthread_rwlock_wrlock(&crops_lock);

	if (crops.count(id) > 0) {
		pthread_rwlock_unlock(&crops_lock);
		return FALSE;
	}

	Crop *crop = new Crop(id, crop_width, crop_height, crop_x, crop_y, layer, dst_width, dst_height, dst_x, dst_y, img, &lock);
	crops[id] = crop;

	pthread_rwlock_unlock(&crops_lock);
	return crop;
}

Crop* Stream::get_crop_by_id(uint32_t crop_id)
{
	pthread_rwlock_rdlock(&crops_lock);

	if (crops.count(crop_id) <= 0) {
		pthread_rwlock_unlock(&crops_lock);
		return NULL;
	}

	pthread_rwlock_unlock(&crops_lock);
	return crops[crop_id];
}

int Stream::remove_crop(uint32_t crop_id)
{
	Crop *crop = get_crop_by_id(crop_id);

	if (crop == NULL){
		return FALSE;
	}

	pthread_rwlock_wrlock(&crops_lock);
	crops.erase(crop_id);
	pthread_rwlock_unlock(&crops_lock);

	crop->stop();
	delete crop;

	return TRUE;
}

int Stream::introduce_frame(uint8_t* buffer, uint32_t buffer_length)
{	
	
	//img.data = (uint8_t*)buffer;
	memcpy((uint8_t*)img.data,(uint8_t*)buffer, buffer_length);
	wake_up_crops();
	
	return TRUE;

}

void Stream::wake_up_crops()
{
	pthread_rwlock_rdlock(&crops_lock);
	for (it = crops.begin(); it != crops.end(); it++){
		it->second->set_new_frame(TRUE);
	}
	pthread_rwlock_unlock(&crops_lock);
}

map<uint32_t, Crop*> Stream::get_crops()
{
	return crops;
}

pthread_rwlock_t* Stream::get_lock()
{
	return &lock;
}

Mat Stream::get_img()
{
	return img;
}

uint32_t Stream::get_width()
{
	return sz.width;
}
		
uint32_t Stream::get_height()
{
	return sz.height;
}

uint32_t Stream::get_id()
{
	return id;
}





