//============================================================================
// Name        : Mixer.cpp
// Author      : 
// Version     :
// Copyright   :
// Description :
//============================================================================

#include <libavcodec/avcodec.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <stream.h>
#include <layout.h>
#include <mutex_object.h>
#include <math.h>
#include <iostream>

using namespace std;

Layout::Layout(int width, int height, enum PixelFormat colorspace, int max_str){
	init_layout(width, height, colorspace, max_str);
}

int Layout::init_layout(int width, int height, enum PixelFormat colorspace, int max_str){
	//Check if width, height and color space are valid
	if (!check_init_layout(width, height, colorspace, max_streams)){
		return -1;
	}

	//Fill layout fields
	lay_width = width;
	lay_height = height;
	max_streams = max_str;
	n_streams = 0;
	max_layers = max_str;
	active_streams_id.reserve(max_streams);
	free_streams_id.reserve(max_streams);
	lay_colorspace = colorspace;
	layout_frame = NULL;

	for (i=0; i<max_streams; i++){
		Stream* stream = new Stream();
		pthread_t *thr;

		stream->set_id(i);
		stream->set_orig_w(0);
		stream->set_orig_h(0);
		stream->set_curr_w(0);
		stream->set_curr_h(0);
		stream->set_x_pos(0);
		stream->set_y_pos(0);
		stream->set_layer(0);
		stream->set_orig_cp(PIX_FMT_YUV420P);
		stream->set_curr_cp(PIX_FMT_RGB24);
		stream->set_orig_frame(avcodec_alloc_frame());
		stream->set_current_frame(avcodec_alloc_frame());
		stream->set_needs_displaying(true);
		stream->set_thread(thr);

		streams[i] = stream;

		//Thread creation
		pthread_create(stream->get_thread(), NULL, Stream::execute_resize, stream);
	}

	return 0;
}

int Layout::modify_layout (int width, int height, enum PixelFormat colorspace, bool resize_streams){
	//Check if width, height and color space are valid
	if (!check_modify_layout(width, height, colorspace)){
		return -1;
	}

	//Check if we have to resize all the streams
	if (resize_streams){
		int i;
		int delta_w = width/lay_width;
		int delta_h = height/lay_height;

		//Modify streams fields
		for (i=0; i<n_streams; i++){
			streams[active_streams_id[i]]->set_curr_w(streams[active_streams_id[i]]->get_curr_w()*delta_w);
			streams[active_streams_id[i]]->set_curr_h(streams[active_streams_id[i]]->get_curr_h()*delta_h);
		}

		//Modify layout fields
		lay_width = width;
		lay_height = height;

		return 0;

	} else{
		//Modify layout fields
		lay_width = width;
		lay_height = height;

		return 0;
	}

}

int Layout::introduce_frame(int stream_id, int width, int height, enum PixelFormat colorspace, uint8_t *data_buffer){
	int id;
	//Check if id is active
	id = check_active_stream (stream_id);
	if (id==-1){
		return -1; //selected stream is not active
	}
	//Check if width, height and color space are valid
	if (!check_introduce_frame_values(width, height, colorspace)){
		return -1; //Data introduced is not valid
	}
	//Check if *data_buffer is not null
	if (!data_buffer){
		return -1; //data_buffer is empty
	}

	pthread_mutex_lock(streams[id]->get_resize_mutex());

	//Check if original frame values have changed
	if (streams[id]->get_orig_w() != width){
		streams[id]->set_orig_w(width);
	}
	if (streams[id]->get_orig_h() != height){
		streams[id]->set_orig_w(width);
	}
	if (streams[id]->get_orig_cp() != colorspace){
		streams[id]->set_orig_cp(colorspace);
	}

	//Fill AVFrame structure
	avpicture_fill((AVPicture *)streams[id]->get_orig_frame(), data_buffer,
			streams[id]->get_orig_cp(), streams[id]->get_orig_w(), streams[id]->get_orig_h());
	pthread_mutex_unlock(streams[id]->get_resize_mutex());

	pthread_mutex_lock(streams[id]->get_orig_frame_ready_mutex());
	streams[id]->set_orig_frame_ready(true);
	pthread_cond_signal(streams[id]->get_orig_frame_ready_cond());
	pthread_mutex_unlock(streams[id]->get_orig_frame_ready_mutex());

	streams[id]->set_needs_displaying(true);

	return 0;
}

int Layout::resize_frame(int stream_id){
	int id;
	//Check if id is active
	id = check_active_stream (stream_id);

	if (id==-1){
		return -1; //Stream is not active
	}
}

int Layout::update_frame(int stream_id){
	int id;
	//Check if id is active
	id = check_active_stream (stream_id);

	if (id==-1){
		return -1; //Stream is not active
	}

	//Wake up resizer thread
}

int Layout::merge_frames(){
	//For every active stream, take resized AVFrames and merge them layer by layer if necessary
	for (i=0; i<max_layers; i++){
		for (j=0; j<=active_streams_id.size(); j++){
			if (i==streams[j]->get_layer() && streams[j]->get_needs_displaying()){

				pthread_mutex_lock(streams[j]->get_resize_mutex());

				print_frame (streams[j]->get_x_pos(), streams[j]->get_y_pos(), streams[j]->get_curr_w(),
						streams[j]->get_curr_h(), streams[j]->get_current_frame(), layout_frame);

				pthread_mutex_unlock(streams[j]->get_resize_mutex());
				streams[j]->set_needs_displaying(false);
			}
		}
	}

	return 0;
}

int Layout::introduce_stream (int orig_w, int orig_h, enum PixelFormat orig_cp, int new_w, int new_h, enum  PixelFormat new_cp){
	int id;
	//Check if width, height and color space are valid
	if (!check_introduce_stream_values(orig_w, orig_h, orig_cp, new_w, new_h, new_cp)){
		return -1;
	}
	//Check if there are available threads
	if (free_streams_id.size() == max_streams){
		return -1; //There are no free streams
	}
	//Update stream id arrays
	id = free_streams_id[free_streams_id.back()];
	active_streams_id.push_back(id);
	free_streams_id.pop_back();

	//Fill stream fields
	streams[id]->set_id(id);
	streams[id]->set_orig_w(orig_w);
	streams[id]->set_orig_h(orig_h);
	streams[id]->set_orig_cp(orig_cp);
	streams[id]->set_curr_w(new_w);
	streams[id]->set_curr_h(new_h);
	streams[id]->set_curr_cp(new_cp);

	return 0;
}

int Layout::modify_stream (int stream_id, int width, int height, enum PixelFormat colorspace, int x_pos, int y_pos, int layer, bool keepAspectRatio){

	int id;
	//Check if id is active
	id = check_active_stream(stream_id);
	if (id==-1){
		return -1; //selected stream is not active
	}
	//Check if width, height, color space, xpos, ypos, layer are valid
	if (!check_modify_stream_values(width, height, colorspace, x_pos, y_pos, layer)){
		return -1;
	}

	pthread_mutex_lock(streams[id]->get_resize_mutex());

	//Modify stream features (the ones which have changed)
	if (width !=NULL){
		streams[id]->set_curr_w(width);
	}
	if (height !=NULL){
		streams[id]->set_curr_h(height);
	}
	if (colorspace !=NULL){
		streams[id]->set_curr_cp(colorspace);
	}
	if (x_pos !=NULL){
		streams[id]->set_x_pos(x_pos);
	}
	if (y_pos !=NULL){
		streams[id]->set_y_pos(y_pos);
	}
	if (width !=NULL){
		streams[id]->set_layer(layer);
	}

	if (keepAspectRatio){
		//modify curr h in order to keep the same aspect ratio of orig_w and orig_h
		float orig_aspect_ratio;
		int delta;
		orig_aspect_ratio = streams[id]->get_orig_w()/streams[id]->get_orig_h();
		delta = floor((streams[id]->get_curr_w() - orig_aspect_ratio*streams[id]->get_curr_h())/orig_aspect_ratio);
		streams[id]->set_curr_h(streams[id]->get_curr_h() + delta);
	}

	pthread_mutex_unlock(streams[id]->get_resize_mutex());

	return 0;
}

int Layout::remove_stream (int stream_id){

	int id;

	//Check if id is active
	id = check_active_stream (stream_id);
	if (id==-1){
		return -1; //selected stream is not active
	}

	//Update stream id arrays
	for (i=0; i<=active_streams_id.size(); i++){
		if (active_streams_id[i] == stream_id){
			free_streams_id.push_back(stream_id);
			active_streams_id.erase(active_streams_id.begin() + i);
			return 0;
		}
	}

	return 0;
}

uint8_t** Layout::get_layout_bytestream(){

	for (i=0; i<=active_streams_id.size(); i++){
		MutexObject mutex(streams[i]->get_resize_mutex());
	}

	return layout_frame->data;
}

int Layout::check_active_stream (int stream_id){

	int id;
	for (i=0; i<n_streams; i++){
		if(active_streams_id[i] == stream_id){
			id = active_streams_id[i];
			return id;
		}
	}
	return -1; //Stream is not active
}

int Layout::print_frame(int x_pos, int y_pos, int width, int height, AVFrame *stream_frame, AVFrame *layout_frame){

	int x, y, contTFrame, contSFrame, max_x, max_y, byte_init_point, byte_offset_line;
	x=0, y=0;
	contTFrame = 0;
	contSFrame = 0;
	max_x = width;
	max_y = height;
	if (width + x_pos > lay_width){
		max_x = width - x_pos;
	}
	if (height + y_pos > lay_height){
		max_y = height - x_pos;
	}
	byte_init_point = y_pos*layout_frame->linesize[0] + x_pos*3;  //Per 3 because every pixel is represented by 3 bytes
	byte_offset_line = layout_frame->linesize[0] - max_x*3; //Per 3 because every pixel is represented by 3 bytes
	contTFrame = byte_init_point;
	for (y = 0 ; y < max_y; y++) {
		for (x = 0; x < max_x; x++) {
			layout_frame->data[0][contTFrame] = stream_frame->data[0][contSFrame];				//R
	    	layout_frame->data[0][contTFrame + 1] = stream_frame->data[0][contSFrame + 1];		//G
	    	layout_frame->data[0][contTFrame + 2] = stream_frame->data[0][contSFrame + 2];		//B
	    	contTFrame += 3;
	    	contSFrame += 3;
	    }
	    contTFrame += byte_offset_line;
	    contSFrame += (width - max_x)*3;
	}

	return 0;
}



bool check_init_layout(int width, int height, enum PixelFormat colorspace, int max_streams){
	if (width <= 0 || height <= 0){
		return false;
	}
	if (max_streams <= 0 || max_streams > MAX_STREAMS){
		return false;
	}
	//TODO		if (colorspace)
	return true;
}


	bool Layout::check_modify_stream_values(int width, int height, enum PixelFormat colorspace, int x_pos, int y_pos, int layer){
		if (width != NULL && (width<=0 || width>lay_width)){
			return false;
		}
		if (height != NULL && (height<=0 || width>lay_height)){
			return false;
		}
//TODO		if (colorspace != NULL && (height<0 || width>lay_height)){
//			return false;
//		}
		if (x_pos != NULL && (x_pos<0 || x_pos>=lay_width)){
			return false;
		}
		if (y_pos != NULL && (y_pos<0 || y_pos>=lay_height)){
			return false;
		}
		if (layer != NULL && (layer<0 || layer>max_layers)){
			return false;
		}
		return true;
	}

	bool Layout::check_introduce_stream_values(int orig_w, int orig_h, enum PixelFormat orig_cp, int new_w, int new_h, enum  PixelFormat new_cp){
		if (orig_w <= 0 || orig_h <= 0){
			return false;
		}
//TODO		if (orig_cp || new_cp)
		if (new_w <=0 || new_w > lay_width){
			return false;
		}
		if (new_h <=0 || new_h > lay_height){
			return false;
		}
		return true;
	}

	bool check_introduce_frame_values (int width, int height, enum PixelFormat colorspace){
		if (width <= 0 || height <= 0){
			return false;
		}
//TODO		if (colorspace)
		return true;
	}

	bool check_modify_layout (int width, int height, enum PixelFormat colorspace){
		if (width <= 0 || height <= 0){
			return false;
		}
//TODO		if (colorspace)
		return true;
	}
