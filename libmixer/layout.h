/*
 * layout.h
 *
 *  Created on: Jun 13, 2013
 *      Author: palau
 */

#ifndef LAYOUT_H_
#define LAYOUT_H_

#include "stream.h"
#include <map>
#include <vector>

#ifndef TRUE
#define FALSE    0
#define TRUE    1
#endif /* TRUE */

#define MAX_STREAMS 8

class Stream;
class Crop;

class Layout {

	private:
		uint32_t lay_width, lay_height;
        Mat layout_img;
        Stream *out_stream;
        multimap<uint32_t, Crop*> crops_by_layers;
        map<uint32_t, Stream*> streams;
        pthread_rwlock_t layers_lock;
        pthread_rwlock_t streams_lock;
        pthread_rwlock_t layout_img_lock;

	public:
        Layout(uint32_t width, uint32_t height);
        ~Layout();

        int add_stream(uint32_t stream_id, uint32_t width, uint32_t height);
        Stream *get_stream_by_id(uint32_t stream_id);
        int remove_stream(uint32_t stream_id);

        int introduce_frame_to_stream(uint32_t stream_id, uint8_t* buffer, uint32_t buffer_length);

        int add_crop_to_stream(uint32_t stream_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
                    uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y);
        int remove_crop_from_stream(uint32_t stream_id, uint32_t crop_id);
        int modify_orig_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y);
        int modify_dst_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y, uint32_t new_layer);

        int enable_crop_from_stream(uint32_t stream_id, uint32_t crop_id);
        int disable_crop_from_stream(uint32_t stream_id, uint32_t crop_id);

        Stream *get_out_stream();

        uint8_t* get_buffer();
        uint32_t get_buffer_size();

        void compose_layout();

        uint8_t check_values(uint32_t max_width, uint32_t max_height, uint32_t width, uint32_t height, uint32_t x, uint32_t y);


};


#endif /* LAYOUT_H_ */
