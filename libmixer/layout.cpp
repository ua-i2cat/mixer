
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libavutil/avutil.h>
}
#include <pthread.h>
#include "stream.h"
#include "layout.h"
#include <math.h>
#include <iostream>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

using namespace std;

Layout::Layout(uint32_t width, uint32_t height, enum AVPixelFormat colorspace, uint32_t max_str){

	//Fill layout fields
	lay_width = width;
	lay_height = height;
	max_streams = max_str;
	max_layers = max_str;
	lay_colorspace = colorspace;
	layout_frame = avcodec_alloc_frame();
	lay_buffsize = avpicture_get_size(lay_colorspace, lay_width, lay_height) * sizeof(uint8_t);
	lay_buffer = (uint8_t*)malloc(lay_buffsize);
	out_buffer = (uint8_t*)malloc(lay_buffsize);
	avpicture_fill((AVPicture *)layout_frame, lay_buffer, lay_colorspace, lay_width, lay_height);
	pthread_rwlock_init(&resize_rwlock, NULL);

}

int Layout::modify_layout (uint32_t width, uint32_t height, enum AVPixelFormat colorspace, bool resize_streams){
	pthread_rwlock_wrlock(&resize_rwlock);
	Stream *stream;

	//Check if width, height and color space are valid
	if (!check_modify_layout(width, height, colorspace)){
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}

	//Check if we have to resize all the streams
	if (resize_streams){
		double w = width;
		double h = height;
		double delta_w = w/lay_width;
		double delta_h = h/lay_height;

		//Modify streams fields
		for ( map<uint32_t,Stream*>::iterator it = streams.begin(); it != streams.end(); it++){
			stream = it->second;
			pthread_mutex_lock(stream->get_in_buffer_mutex());

			stream->set_curr_w(stream->get_curr_w()*delta_w);
			stream->set_curr_h(stream->get_curr_h()*delta_h);
			stream->set_x_pos(stream->get_x_pos()*delta_w);
			stream->set_y_pos(stream->get_y_pos()*delta_h);

			stream->set_buffsize(avpicture_get_size(stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h()) * sizeof(uint8_t));
			free(stream->get_buffer());
			stream->set_buffer((uint8_t*)malloc(stream->get_buffsize()));
			avpicture_fill((AVPicture *)stream->get_current_frame(), stream->get_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

			free(stream->get_dummy_buffer());
			stream->set_dummy_buffer((uint8_t*)malloc(stream->get_buffsize()));
			avpicture_fill((AVPicture *)stream->get_dummy_frame(), stream->get_dummy_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

			sws_freeContext (stream->get_ctx());
			stream->set_ctx(sws_getContext(stream->get_orig_w(), stream->get_orig_h(), stream->get_orig_cp(),
					stream->get_curr_w(), stream->get_curr_h(), stream->get_curr_cp(), SWS_BILINEAR, NULL, NULL, NULL));

			pthread_mutex_unlock(stream->get_in_buffer_mutex());

			pthread_mutex_lock(stream->get_orig_frame_ready_mutex());
			stream->set_orig_frame_ready(true);
			pthread_cond_signal(stream->get_orig_frame_ready_cond());
			pthread_mutex_unlock(stream->get_orig_frame_ready_mutex());
		}

		//Modify layout fields
		lay_width = width;
		lay_height = height;
		lay_buffsize = avpicture_get_size(lay_colorspace, lay_width, lay_height) * sizeof(uint8_t);
		free(lay_buffer);
		free(out_buffer);
		lay_buffer = (uint8_t*)malloc(lay_buffsize);
		out_buffer = (uint8_t*)malloc(lay_buffsize);
		avpicture_fill((AVPicture *)layout_frame, lay_buffer, lay_colorspace, lay_width, lay_height);

	} else{
		//Modify layout fields
		lay_width = width;
		lay_height = height;
		lay_buffsize = avpicture_get_size(lay_colorspace, lay_width, lay_height) * sizeof(uint8_t);
		free(lay_buffer);
		free(out_buffer);
		lay_buffer = (uint8_t*)malloc(lay_buffsize);
		out_buffer = (uint8_t*)malloc(lay_buffsize);
		avpicture_fill((AVPicture *)layout_frame, lay_buffer, lay_colorspace, lay_width, lay_height);

	}

	pthread_rwlock_unlock(&resize_rwlock);

	merge_frames();

	return 0;

}

int Layout::introduce_frame(uint32_t id, uint8_t *data_buffer, uint32_t data_length){
	
	if (streams.count(id) <= 0) {
		return -1;
	}

	//Check if *data_buffer is not null
	if (data_buffer == NULL){
		return -1; //data_buffer is empty
	}

	pthread_mutex_lock(streams[id]->get_in_buffer_mutex());

	assert(data_length == streams[id]->get_in_buffsize());
	memcpy((uint8_t*)streams[id]->get_in_buffer(),(uint8_t*)data_buffer, data_length);
	pthread_mutex_unlock(streams[id]->get_in_buffer_mutex());

	pthread_mutex_lock(streams[id]->get_orig_frame_ready_mutex());
	streams[id]->set_orig_frame_ready(true);
	pthread_cond_signal(streams[id]->get_orig_frame_ready_cond());
	pthread_mutex_unlock(streams[id]->get_orig_frame_ready_mutex());

	return 0;
}

int Layout::merge_frames(){

	Stream *stream;

	//For every active stream, take resized AVFrames and merge them layer by layer
	for (multimap<uint32_t, uint32_t>::iterator it = layers.begin(); it != layers.end(); it++){
		pthread_rwlock_wrlock(&resize_rwlock);
		if (streams[it->second]->get_active() == 1 && streams[it->second]->get_x_pos()<lay_width && streams[it->second]->get_y_pos()<lay_height){
			
			stream = streams[it->second];	
			print_frame(stream->get_x_pos(), stream->get_y_pos(), stream->get_curr_w(),
						stream->get_curr_h(), stream->get_current_frame(), layout_frame);

		}
		pthread_rwlock_unlock(&resize_rwlock);
	}

	return 0;
}

int Layout::introduce_stream (uint32_t id, uint32_t orig_w, uint32_t orig_h, enum AVPixelFormat orig_cp, 
	uint32_t new_w, uint32_t new_h, enum AVPixelFormat new_cp, uint32_t x, uint32_t y, uint32_t layer){

	if (!check_introduce_stream_values(orig_w, orig_h, orig_cp, new_w, new_h, new_cp, x, y)){
		return -1; //Introduced values are not correct
	}

	if (streams.size() == max_streams){
		return -1; //We have reached the max number of streams
	}

	if(streams.count(id) > 0){
		return -1;  //ID already exists
	}

	Stream *stream = new Stream(id, &resize_rwlock, orig_w, orig_h, orig_cp, new_w, new_h, new_cp, x, y, layer);

	if (stream == NULL){
		return -1;
	}

	pthread_rwlock_wrlock(&resize_rwlock);
	streams[id] = stream;
	stream->set_active(1);
	layers.insert(pair<uint32_t, uint32_t>(layer,id));
	pthread_rwlock_unlock(&resize_rwlock);

	return 0;
}

int Layout::modify_stream (uint32_t stream_id, uint32_t width, uint32_t height, enum AVPixelFormat colorspace, 
							uint32_t x_pos, uint32_t y_pos, uint32_t layer, bool keepAspectRatio){

	pthread_rwlock_wrlock(&resize_rwlock);

//TODO: resize rwlock podria ser un read lock si fem un wrlock d'un mutex intern de l'stream
	
	if (streams.count(stream_id) <= 0) {
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}

	//Check if width, height,color space, xpos, ypos, layer are valid
	if (!check_modify_stream_values(width, height, colorspace, x_pos, y_pos, layer)){
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}

	Stream *stream = streams[stream_id];
	uint32_t old_x_pos = stream->get_x_pos();
	uint32_t old_y_pos = stream->get_y_pos();
	uint32_t old_width = stream->get_curr_w(); 
	uint32_t old_height = stream->get_curr_h();
	bool size_modified = false, pos_modified = false;

	//Modify stream features (the ones which have changed)
	if (width != stream->get_curr_w()){
		old_width = stream->get_curr_w();
		stream->set_curr_w(width);
		size_modified = true;
	}
	if (height != stream->get_curr_h()){
		old_height = stream->get_curr_h();
		stream->set_curr_h(height);
		size_modified = true;
	}
	if (colorspace != stream->get_curr_cp()){
		stream->set_curr_cp(colorspace);
	}
	if (x_pos != stream->get_x_pos()){
		old_x_pos = stream->get_x_pos();
		stream->set_x_pos(x_pos);
		pos_modified = true;
	}
	if (y_pos != stream->get_y_pos()){
		old_y_pos = stream->get_y_pos();
		stream->set_y_pos(y_pos);
		pos_modified = true;
	}
	if (layer != stream->get_layer()){
		std::multimap<uint32_t,uint32_t>::iterator it = layers.find(stream->get_layer());  
		while (it->second != stream->get_id()){
			it++;
		}
		layers.erase(it);
		stream->set_layer(layer);
		layers.insert(pair<uint32_t, uint32_t>(layer,stream_id));
	}

	if (keepAspectRatio){
		//modify curr h in order to keep the same aspect ratio of orig_w and orig_h
		float orig_aspect_ratio;
		int delta;
		if (size_modified == false){
			old_width = stream->get_curr_w();
			old_height = stream->get_curr_h();
		}
		orig_aspect_ratio = stream->get_orig_w()/stream->get_orig_h();
		delta = floor((stream->get_curr_w() - orig_aspect_ratio*stream->get_curr_h())/orig_aspect_ratio);
		stream->set_curr_h(stream->get_curr_h() + delta);
		size_modified = true;
	}

	if (size_modified && !pos_modified){
		stream->set_dummy_buffer((uint8_t*)realloc(stream->get_dummy_buffer(), avpicture_get_size(stream->get_curr_cp(), old_width, old_height) * sizeof(uint8_t)));
		avpicture_fill((AVPicture *)stream->get_dummy_frame(), stream->get_dummy_buffer(), stream->get_curr_cp(), old_width, old_height);

		stream->set_buffsize(avpicture_get_size(stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h()) * sizeof(uint8_t));
		stream->set_buffer((uint8_t*)realloc(stream->get_buffer(),stream->get_buffsize()));
		avpicture_fill((AVPicture *)stream->get_current_frame(), stream->get_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

		print_frame(stream->get_x_pos(), stream->get_y_pos(), old_width, old_height, stream->get_dummy_frame(), layout_frame);

		stream->set_dummy_buffer((uint8_t*)realloc(stream->get_dummy_buffer(), stream->get_buffsize()));
		avpicture_fill((AVPicture *)stream->get_dummy_frame(), stream->get_dummy_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

		sws_freeContext (stream->get_ctx());
		stream->set_ctx(sws_getContext(stream->get_orig_w(), stream->get_orig_h(), stream->get_orig_cp(),
				stream->get_curr_w(), stream->get_curr_h(), stream->get_curr_cp(), SWS_BILINEAR, NULL, NULL, NULL));


	} else if (!size_modified && pos_modified){
		print_frame(old_x_pos, old_y_pos, stream->get_curr_w(), stream->get_curr_h(), stream->get_dummy_frame(), layout_frame);

	} else if (size_modified && pos_modified){
		stream->set_dummy_buffer((uint8_t*)realloc(stream->get_dummy_buffer(), avpicture_get_size(stream->get_curr_cp(), old_width, old_height) * sizeof(uint8_t)));
		avpicture_fill((AVPicture *)stream->get_dummy_frame(), stream->get_dummy_buffer(), stream->get_curr_cp(), old_width, old_height);

		stream->set_buffsize(avpicture_get_size(stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h()) * sizeof(uint8_t));
		stream->set_buffer((uint8_t*)realloc(stream->get_buffer(),stream->get_buffsize()));
		avpicture_fill((AVPicture *)stream->get_current_frame(), stream->get_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

		print_frame(old_x_pos, old_y_pos, old_width, old_height, stream->get_dummy_frame(), layout_frame);

		stream->set_dummy_buffer((uint8_t*)realloc(stream->get_dummy_buffer(), stream->get_buffsize()));
		avpicture_fill((AVPicture *)stream->get_dummy_frame(), stream->get_dummy_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

		sws_freeContext (stream->get_ctx());
		stream->set_ctx(sws_getContext(stream->get_orig_w(), stream->get_orig_h(), stream->get_orig_cp(),
				stream->get_curr_w(), stream->get_curr_h(), stream->get_curr_cp(), SWS_BILINEAR, NULL, NULL, NULL));
	}
	
	pthread_rwlock_unlock(&resize_rwlock);

	return 0;
}

int Layout::remove_stream (uint32_t id){
	pthread_rwlock_wrlock(&resize_rwlock);
	if (streams.count(id) <= 0) {
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}
	Stream *stream = streams[id];

	print_frame(stream->get_x_pos(), stream->get_y_pos(), stream->get_curr_w(), stream->get_curr_h(), stream->get_dummy_frame(), layout_frame);

	std::multimap<uint32_t,uint32_t>::iterator it = layers.find(stream->get_layer());  
	while (it->second != id){
		it++;
	}
	layers.erase(it);
	
	delete stream;
	streams.erase(id);
	pthread_rwlock_unlock(&resize_rwlock);

	return 0;
}

uint8_t* Layout::get_layout_bytestream(){
	pthread_rwlock_wrlock(&resize_rwlock);
	avpicture_layout((AVPicture *)layout_frame, lay_colorspace, lay_width, lay_height, out_buffer, lay_buffsize);
	pthread_rwlock_unlock(&resize_rwlock);
	return out_buffer;
}

int Layout::set_active(uint32_t id, uint8_t active_flag){
	pthread_rwlock_wrlock(&resize_rwlock);
	
	if (streams.count(id) <= 0) {
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}

	if (active_flag == 1){ //enable stream
		memcpy (streams[id]->get_buffer(), streams[id]->get_dummy_buffer(), streams[id]->get_buffsize());
		streams[id]->set_active(1);
		pthread_rwlock_unlock(&resize_rwlock);
		return 0;
	} else if (active_flag == 0){ //disable stream
		streams[id]->set_active(0);
		print_frame(streams[id]->get_x_pos(), streams[id]->get_y_pos(), streams[id]->get_curr_w(), streams[id]->get_curr_h(), streams[id]->get_dummy_frame(), layout_frame);
		memcpy (streams[id]->get_buffer(), streams[id]->get_dummy_buffer(), streams[id]->get_buffsize());
		pthread_rwlock_unlock(&resize_rwlock);
		return 0;
	} else {
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}
}

int Layout::print_frame(uint32_t x_pos, uint32_t y_pos, uint32_t width, uint32_t height, AVFrame *stream_frame, AVFrame *layout_frame){

	uint32_t y, contTFrame, contSFrame, max_x, max_y, byte_init_point, byte_offset_line;

	y=0;
	contTFrame = 0;
	contSFrame = 0;
	max_x = width;
	max_y = height;
	if (width + x_pos > lay_width){
		max_x = lay_width - x_pos;
	}
	if (height + y_pos > lay_height){
		max_y = lay_height - y_pos;
	}

	byte_init_point = y_pos*layout_frame->linesize[0] + x_pos*3;  //Per 3 because every pixel is represented by 3 bytes
	byte_offset_line = layout_frame->linesize[0] - max_x*3; //Per 3 because every pixel is represented by 3 bytes
	contTFrame = byte_init_point;

	for (y = 0; y < max_y; y++) {
	    memcpy(&layout_frame->data[0][contTFrame], &stream_frame->data[0][contSFrame], stream_frame->linesize[0] - (width - max_x)*3);
	    contTFrame += byte_offset_line + 3*max_x;
	    contSFrame += (width - max_x)*3 + 3*max_x;
	}

	return 0;
}

bool Layout::check_modify_stream_values(uint32_t width, uint32_t height, enum AVPixelFormat colorspace, uint32_t x_pos, uint32_t y_pos, uint32_t layer){
	if (width == 0){
		return false;
	}
	if (height == 0){
		return false;
	}
	if (x_pos >= lay_width){
		return false;
	}
	if (y_pos>=lay_height){
		return false;
	}
	if (layer>max_layers){
		return false;
	}
	return true;
}

bool Layout::check_introduce_stream_values(uint32_t orig_w, uint32_t orig_h, enum AVPixelFormat orig_cp, uint32_t new_w, 
											uint32_t new_h, enum  AVPixelFormat new_cp, uint32_t x, uint32_t y){
	if (orig_w < 0 || orig_h < 0){
		return false;
	}
	if (new_w <=0){
		return false;
	}
	if (new_h <=0){
		return false;
	}
	if (x<0 || x>=lay_width){
		return false;
	}
	if (y<0 || y>=lay_height){
		return false;
	}

	return true;
}

bool Layout::check_introduce_frame_values (uint32_t width, uint32_t height, enum AVPixelFormat colorspace){
	if (width <= 0 || height <= 0){
		return false;
	}
	return true;
}

bool Layout::check_modify_layout (uint32_t width, uint32_t height, enum AVPixelFormat colorspace){
	if (width <= 0 || height <= 0){
		return false;
	}
	return true;
}

uint32_t Layout::get_w(){
	return lay_width;
}

uint32_t Layout::get_h(){
	return lay_height;
}

AVFrame* Layout::get_lay_frame(){
	return layout_frame;
}

std::vector<uint32_t> Layout::get_streams_id(){
	std::vector<uint32_t> v;
	for (multimap<uint32_t, uint32_t>::iterator it = layers.begin(); it != layers.end(); it++){
		v.push_back(it->second);
	}
	return v;

}

Stream* Layout::get_stream(uint32_t stream_id){
	std::map<uint32_t, Stream*>::iterator it = streams.find(stream_id);
	if (it == streams.end()){
		return NULL;
	}
	return it->second;
}

uint8_t Layout::check_if_layout_stream(uint32_t id){
	std::map<uint32_t, Stream*>::iterator it = streams.find(id);
	if (it == streams.end()){
		return FALSE;
	}
	return TRUE;
}

uint32_t Layout::get_buffsize(){
	return lay_buffsize;
}

uint32_t Layout::get_max_streams(){
	return max_streams;
}

Layout::~Layout(){
  	
  	for (map<uint32_t,Stream*>::iterator it = streams.begin(); it != streams.end(); it++){
  		pthread_cancel(it->second->get_thread());
  		pthread_join(it->second->get_thread(), NULL);
  		delete it->second;
  	}

  	if (layout_frame != NULL)
  		avcodec_free_frame(&layout_frame);
  	if (lay_buffer != NULL)
  		free(lay_buffer);
  	if (out_buffer != NULL)
  		free(out_buffer);
}
