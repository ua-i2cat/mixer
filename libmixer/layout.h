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






        enum PixelFormat lay_colorspace;
        AVFrame *layout_frame;
        uint8_t *lay_buffer, *out_buffer;
        unsigned int lay_buffsize;
        pthread_rwlock_t resize_rwlock;
        pthread_t* thr;

	    bool check_init_layout(int width, int height, enum AVPixelFormat colorspace, int max_streams);
    	int check_active_stream(int stream_id);
    	int print_frame(uint32_t x_pos, uint32_t y_pos, uint32_t width, uint32_t height, AVFrame *stream_frame, AVFrame *layout_frame);
    	bool check_modify_stream_values(uint32_t width, uint32_t height, enum AVPixelFormat colorspace, uint32_t x_pos, uint32_t y_pos, uint32_t layer);
    	bool check_introduce_stream_values (uint32_t orig_w, uint32_t orig_h, enum AVPixelFormat orig_cp, uint32_t new_w, uint32_t new_h, enum  AVPixelFormat new_cp, uint32_t x, uint32_t y);
    	bool check_introduce_frame_values (uint32_t width, uint32_t height, enum AVPixelFormat colorspace);
    	bool check_modify_layout (uint32_t width, uint32_t height, enum AVPixelFormat colorspace);

	public:
        Layout(uint32_t width, uint32_t height);
        ~Layout();

        int add_stream(uint32_t stream_id, uint32_t width, uint32_t height);
        Stream *get_stream_by_id(uint32_t stream_id);
        int remove_stream(uint32_t stream_id);

        int add_crop_to_stream(uint32_t stream_id);
        int remove_crop_from_stream(uint32_t stream_id, uint32_t crop_id);

        Stream *get_out_stream();

        int compose_layout();








        int introduce_frame (uint32_t stream_id, uint8_t *data_buffer, uint32_t data_length);
        int merge_frames();
        int introduce_stream (uint32_t id, uint32_t orig_w, uint32_t orig_h, enum AVPixelFormat orig_cp, uint32_t new_w, 
                                        uint32_t new_h, enum AVPixelFormat new_cp, uint32_t x, uint32_t y, uint32_t layer);
        int introduce_stream (enum AVPixelFormat orig_cp, uint32_t new_w, uint32_t new_h, uint32_t x, uint32_t y, enum  AVPixelFormat new_cp, uint32_t layer);
        int modify_stream (uint32_t stream_id, uint32_t width, uint32_t height, enum PixelFormat colorspace, 
                            uint32_t xpos, uint32_t ypos, uint32_t layer, bool keepAspectRatio);
        int remove_stream (uint32_t stream_id);
        uint8_t* get_layout_bytestream();
        int modify_layout (uint32_t width, uint32_t height, enum PixelFormat colorspace, bool resize_streams);
        uint32_t get_w();
        uint32_t get_h();
        AVFrame* get_lay_frame();
        Stream* get_stream(uint32_t stream_id);
        uint32_t get_buffsize();
        uint32_t get_max_streams();
        int set_active(uint32_t stream_id, uint8_t active_flag);
        std::vector<uint32_t> get_streams_id();
        uint8_t check_if_layout_stream(uint32_t id);

};


#endif /* LAYOUT_H_ */
