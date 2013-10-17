/*
 * layout.h
 *
 *  Created on: Jun 13, 2013
 *      Author: palau
 */

#ifndef LAYOUT_H_
#define LAYOUT_H_

extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
}
#include "stream.h"
#include <map>

#define MAX_STREAMS 8

class Stream;

class Layout {

	private:
		int lay_width, lay_height, max_streams, max_layers, i, j;
	    enum PixelFormat lay_colorspace;
	    AVFrame *layout_frame;
	    bool overlap;
	    uint8_t *lay_buffer, *out_buffer;
	    unsigned int lay_buffsize;
	    pthread_rwlock_t resize_rwlock;
        pthread_t* thr;
        map<uint32_t, *Stream> streams;
        map<uint32_t, *Stream>::iterator it;

	    bool check_init_layout(int width, int height, enum AVPixelFormat colorspace, int max_streams);
    	int check_active_stream(int stream_id);
    	int print_frame(int x_pos, int y_pos, int width, int height, AVFrame *stream_frame, AVFrame *layout_frame);
    	bool check_modify_stream_values(int width, int height, enum AVPixelFormat colorspace, int x_pos, int y_pos, int layer);
    	bool check_introduce_stream_values (int orig_w, int orig_h, enum AVPixelFormat orig_cp, int new_w, int new_h, enum  AVPixelFormat new_cp, int x, int y);
    	bool check_introduce_frame_values (int width, int height, enum AVPixelFormat colorspace);
    	bool check_modify_layout (int width, int height, enum AVPixelFormat colorspace);
    	bool check_frame_overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    	bool check_overlap();
    	void print_active_stream_id();
    	void print_free_stream_id();

	public:
    	int introduce_frame (int stream_id, uint8_t *data_buffer, int data_length);
    	int merge_frames();
    	int introduce_stream (int orig_w, int orig_h, enum PixelFormat orig_cp, int new_w, int new_h, int x, int y, enum PixelFormat new_cp, int layer);
        int introduce_stream (enum AVPixelFormat orig_cp, int new_w, int new_h, int x, int y, enum  AVPixelFormat new_cp, int layer);
    	int modify_stream (int stream_id, int width, int height, enum PixelFormat colorspace, int xpos, int ypos, int layer, bool keepAspectRatio);
        int update_stream(int stream_id, int width, int height);
    	int remove_stream (int stream_id);
    	uint8_t* get_layout_bytestream();
    	int modify_layout (int width, int height, enum PixelFormat colorspace, bool resize_streams);
    	int init(int width, int height, enum AVPixelFormat colorspace, int max_str);
    	int get_w();
    	int get_h();
    	AVFrame* get_lay_frame();
    	Stream* get_stream(int stream_id);
    	std::vector<int> get_streams_id();
    	unsigned int get_buffsize();
    	int get_max_streams();
    	void print_active_stream_info();
        int set_active(int stream_id, uint8_t active_flag);
        ~Layout();
        Layout(int width, int height, enum PixelFormat colorspace, int max_str);

};


#endif /* LAYOUT_H_ */
