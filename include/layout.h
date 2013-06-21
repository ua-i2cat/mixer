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
#include <stream.h>
#include <vector>

#define MAX_STREAMS 8

class Stream;

class Layout {

	private:
		int lay_width, lay_height, max_streams, n_streams, max_layers, i, j;
		std::vector<int> active_streams_id;
	    std::vector<int> free_streams_id;
	    enum PixelFormat lay_colorspace;
	    std::vector<Stream*> streams;
	    AVFrame *layout_frame;
	    bool overlap;
	    pthread_mutex_t* merge_mutex;
	    uint8_t *lay_buffer;
	    unsigned int lay_buffsize;

	    bool check_init_layout(int width, int height, enum AVPixelFormat colorspace, int max_streams);
    	int check_active_stream(int stream_id);
    	int print_frame(int x_pos, int y_pos, int width, int height, AVFrame *stream_frame, AVFrame *layout_frame);
    	bool check_modify_stream_values(int width, int height, enum AVPixelFormat colorspace, int x_pos, int y_pos, int layer);
    	bool check_introduce_stream_values (int orig_w, int orig_h, enum AVPixelFormat orig_cp, int new_w, int new_h, enum  PixelFormat new_cp, int x, int y);
    	bool check_introduce_frame_values (int width, int height, enum AVPixelFormat colorspace);
    	bool check_modify_layout (int width, int height, enum AVPixelFormat colorspace);
    	bool check_frame_overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2);
    	bool check_overlap();


	public:

    	int introduce_frame (int stream_id, int width, int height, enum PixelFormat colorspace, uint8_t *data_buffer);
    	int merge_frames();
    	int introduce_stream (int orig_w, int orig_h, enum PixelFormat orig_cp, int new_w, int new_h, int x, int y, enum PixelFormat new_cp);
    	int modify_stream (int stream_id, int width, int height, enum PixelFormat colorspace, int xpos, int ypos, int layer, bool keepAspectRatio);
    	int remove_stream (int stream_id);
    	uint8_t** get_layout_bytestream();
    	int modify_layout (int width, int height, enum PixelFormat colorspace, bool resize_streams);
    	int init(int width, int height, enum AVPixelFormat colorspace, int max_str);

};


#endif /* LAYOUT_H_ */
