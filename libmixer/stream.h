/*
 * Stream.h
 *
 *  Created on: Jun 13, 2013
 *      Author: palau
 */

#ifndef STREAM_H_
#define STREAM_H_

#include "crop.h"
#include <map>

using namespace cv;
using namespace std;

class Crop;

class Stream {

	private:
		uint32_t id;
		Size sz;
		Mat img;
		map<uint32_t, Crop*> crops;
		pthread_rwlock_t crops_lock;
		pthread_rwlock_t lock;
		pthread_cond_t new_frame_cond;
		pthread_mutex_t new_frame_lock;
		uint8_t new_frame;

	public:
		Stream(uint32_t stream_id, uint32_t stream_width, uint32_t stream_height); 
		Crop* add_crop(uint32_t id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y,
				uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y);
		Crop* get_crop_by_id(uint32_t crop_id);
		int remove_crop(uint32_t crop_id);
		int introduce_frame(uint8_t* buffer, uint32_t buffer_length);

		map<uint32_t, Crop*> get_crops();
		pthread_rwlock_t* get_lock();

};



#endif /* STREAM_H_ */
