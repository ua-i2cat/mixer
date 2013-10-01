
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

using namespace std;

Layout::Layout(int width, int height, enum PixelFormat colorspace, int max_str){

	//Fill layout fields
	lay_width = width;
	lay_height = height;
	max_streams = max_str;
	max_layers = max_str;
	streams.reserve(max_streams);
	active_streams_id.reserve(max_streams);
	free_streams_id.reserve(max_streams);
	lay_colorspace = colorspace;
	layout_frame = avcodec_alloc_frame();
	lay_buffsize = avpicture_get_size(lay_colorspace, lay_width, lay_height) * sizeof(uint8_t);
	lay_buffer = (uint8_t*)malloc(lay_buffsize);
	out_buffer = (uint8_t*)malloc(lay_buffsize);
	avpicture_fill((AVPicture *)layout_frame, lay_buffer, lay_colorspace, lay_width, lay_height);
	overlap = false;
	pthread_rwlock_init(&resize_rwlock, NULL);

	pthread_t thr[max_streams];

	for (i=0; i<max_streams; i++){

		free_streams_id.push_back(i);

		Stream* stream = new Stream(i, thr[i], &resize_rwlock);
		streams[i] = stream;

		//Thread creation
		pthread_create(&thr[i], NULL, Stream::execute_resize, stream);
	}
}

int Layout::modify_layout (int width, int height, enum AVPixelFormat colorspace, bool resize_streams){
	pthread_rwlock_wrlock(&resize_rwlock);
	Stream *stream;

	//Check if width, height and color space are valid
	if (!check_modify_layout(width, height, colorspace)){
#ifdef ENABLE_DEBUG
		cout << "Layout modification failed: introduced values not valid" << endl;
#endif
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}

	//Check if we have to resize all the streams
	if (resize_streams){
		int i;
		double w = width;
		double h = height;
		double delta_w = w/lay_width;
		double delta_h = h/lay_height;

		//Modify streams fields
		for (i=0; i<(int)active_streams_id.size(); i++){
			pthread_mutex_lock(streams[active_streams_id[i]]->get_in_buffer_mutex());
			stream = streams[active_streams_id[i]];

			stream->set_curr_w(stream->get_curr_w()*delta_w);
			stream->set_curr_h(stream->get_curr_h()*delta_h);
			stream->set_x_pos(stream->get_x_pos()*delta_w);
			stream->set_y_pos(stream->get_y_pos()*delta_h);

			stream->set_buffsize(avpicture_get_size(stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h()) * sizeof(uint8_t));
			free(stream->get_buffer());
			stream->set_buffer((uint8_t*)malloc(*stream->get_buffsize()));
			avpicture_fill((AVPicture *)stream->get_current_frame(), stream->get_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

			free(stream->get_dummy_buffer());
			stream->set_dummy_buffer((uint8_t*)malloc(*stream->get_buffsize()));
			avpicture_fill((AVPicture *)stream->get_dummy_frame(), stream->get_dummy_buffer(), stream->get_curr_cp(), stream->get_curr_w(), stream->get_curr_h());

			sws_freeContext (stream->get_ctx());
			stream->set_ctx(sws_getContext(stream->get_orig_w(), stream->get_orig_h(), stream->get_orig_cp(),
					stream->get_curr_w(), stream->get_curr_h(), stream->get_curr_cp(), SWS_BILINEAR, NULL, NULL, NULL));

			pthread_mutex_unlock(streams[active_streams_id[i]]->get_in_buffer_mutex());

			pthread_rwlock_wrlock(stream->get_needs_displaying_rwlock());
				stream->set_needs_displaying(true);
			pthread_rwlock_unlock(stream->get_needs_displaying_rwlock());

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

#ifdef ENABLE_DEBUG
		cout << "Resizing of streams and layout succeed" << endl;
#endif

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

		for (i=0; i<(int)active_streams_id.size(); i++){
			pthread_rwlock_wrlock(streams[active_streams_id[i]]->get_needs_displaying_rwlock());
			streams[active_streams_id[i]]->set_needs_displaying(true);
			pthread_rwlock_unlock(streams[active_streams_id[i]]->get_needs_displaying_rwlock());
		}

#ifdef ENABLE_DEBUG
		cout << "Resizing of layout succeed" << endl;
#endif

	}

	pthread_rwlock_unlock(&resize_rwlock);

	merge_frames();

	return 0;

}

int Layout::introduce_frame(int stream_id, uint8_t *data_buffer, int data_length){
	int id;
	//Check if id is active
	id = check_active_stream(stream_id);
	if (id==-1){
#ifdef ENABLE_DEBUG
		cout << "Stream " << stream_id << " in not active" << endl;
#endif
		return -1; //selected stream is not active
	}

	//Check if *data_buffer is not null
	if (data_buffer == NULL){
#ifdef ENABLE_DEBUG
		printf("Data buffer is null.");
#endif
		return -1; //data_buffer is empty
	}

	pthread_mutex_lock(streams[id]->get_in_buffer_mutex());

	assert(data_length == streams[id]->get_in_buffsize());
	memcpy((uint8_t*)streams[id]->get_in_buffer(),(uint8_t*)data_buffer, data_length);
	pthread_mutex_unlock(streams[id]->get_in_buffer_mutex());

#ifdef ENABLE_DEBUG
	cout << "Stream " << stream_id << " has introduced a new frame" << endl;
#endif

	pthread_rwlock_wrlock(streams[id]->get_needs_displaying_rwlock());
		streams[id]->set_needs_displaying(true);
	pthread_rwlock_unlock(streams[id]->get_needs_displaying_rwlock());

	pthread_mutex_lock(streams[id]->get_orig_frame_ready_mutex());
	streams[id]->set_orig_frame_ready(true);
	pthread_cond_signal(streams[id]->get_orig_frame_ready_cond());
	pthread_mutex_unlock(streams[id]->get_orig_frame_ready_mutex());

	return 0;
}

int Layout::merge_frames(){

	Stream *stream;

	if(overlap){
#ifdef ENABLE_DEBUG
		cout << "Overlap detected: we are printing every frame again" << endl;
#endif
		//For every active stream, take resized AVFrames and merge them layer by layer
		for (i=0; i<max_layers; i++){
			pthread_rwlock_wrlock(&resize_rwlock);
			for (j=0; j<(int)active_streams_id.size(); j++){
				if (i==streams[active_streams_id[j]]->get_layer() && streams[active_streams_id[j]]->get_active() == 1 &&
						streams[active_streams_id[j]]->get_x_pos()<lay_width && streams[active_streams_id[j]]->get_y_pos()<lay_height){

					stream = streams[active_streams_id[j]];

					print_frame(stream->get_x_pos(), stream->get_y_pos(), stream->get_curr_w(),
						stream->get_curr_h(), stream->get_current_frame(), layout_frame);
#ifdef ENABLE_DEBUG
					cout << "Stream " << stream->get_id() << " frame has been printed into layout" << endl;
#endif

					pthread_rwlock_wrlock(streams[active_streams_id[j]]->get_needs_displaying_rwlock());
					streams[active_streams_id[j]]->set_needs_displaying(false);
					pthread_rwlock_unlock(streams[active_streams_id[j]]->get_needs_displaying_rwlock());
				}
			}
			pthread_rwlock_unlock(&resize_rwlock);
		}
		
	} else{

#ifdef ENABLE_DEBUG
		cout << "There's no overlaping: we only update the frames that need it" << endl;
#endif
		//For every active stream, check if needs to be desplayed and print it
		for (i=0; i<max_layers; i++){
			pthread_rwlock_wrlock(&resize_rwlock);
			for (j=0; j<(int)active_streams_id.size(); j++){
				if (i==streams[active_streams_id[j]]->get_layer() && streams[active_streams_id[j]]->get_needs_displaying()  &&
						streams[active_streams_id[j]]->get_active() == 1 && streams[active_streams_id[j]]->get_x_pos()<lay_width && 
							streams[active_streams_id[j]]->get_y_pos()<lay_height){

					stream = streams[active_streams_id[j]];

					print_frame(stream->get_x_pos(), stream->get_y_pos(), stream->get_curr_w(),
						stream->get_curr_h(), stream->get_current_frame(), layout_frame);

#ifdef ENABLE_DEBUG
					printf("Stream %d frame has been printed into layout\n", stream->get_id());
#endif
					pthread_rwlock_rdlock(stream->get_needs_displaying_rwlock());
					stream->set_needs_displaying(false);
					pthread_rwlock_unlock(stream->get_needs_displaying_rwlock());
				}
			}
			pthread_rwlock_unlock(&resize_rwlock);
		}
	}

#ifdef ENABLE_DEBUG
	printf("Frame merging finished.\n");
#endif

	return 0;
}

int Layout::introduce_stream (int orig_w, int orig_h, enum AVPixelFormat orig_cp, int new_w, int new_h, int x, int y, enum  PixelFormat new_cp, int layer){
	pthread_rwlock_wrlock(&resize_rwlock);

	int id;
	//Check if width, height and color space are valid
	if (!check_introduce_stream_values(orig_w, orig_h, orig_cp, new_w, new_h, new_cp, x, y)){
#ifdef ENABLE_DEBUG
		cout << "Stream introduction failed: introduced values not valid" << endl;
#endif
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}
	//Check if there are available threads
	if ((int)active_streams_id.size() == max_streams){
#ifdef ENABLE_DEBUG
		cout << "Stream introduction failed: the maximum of streams are active" << endl;
#endif
		pthread_rwlock_unlock(&resize_rwlock);
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
	streams[id]->set_x_pos(x);
	streams[id]->set_y_pos(y);
	streams[id]->set_layer(layer);
	streams[id]->set_active(1);

	//Generate AVFrame structures
	streams[id]->set_in_buffsize(avpicture_get_size(streams[id]->get_orig_cp(), streams[id]->get_orig_w(), streams[id]->get_orig_h()) * sizeof(uint8_t));
	streams[id]->set_in_buffer((uint8_t*)malloc(streams[id]->get_in_buffsize()));
	streams[id]->set_buffsize(avpicture_get_size(streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h()) * sizeof(uint8_t));
	streams[id]->set_buffer((uint8_t*)malloc(*streams[id]->get_buffsize()));
	streams[id]->set_dummy_buffer((uint8_t*)malloc(*streams[id]->get_buffsize()));
	avpicture_fill((AVPicture *)streams[id]->get_orig_frame(), streams[id]->get_in_buffer(), streams[id]->get_orig_cp(), streams[id]->get_orig_w(), streams[id]->get_orig_h());
	avpicture_fill((AVPicture *)streams[id]->get_current_frame(), streams[id]->get_buffer(), streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h());
	avpicture_fill((AVPicture *)streams[id]->get_dummy_frame(), streams[id]->get_dummy_buffer(), streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h());
	streams[id]->set_ctx(sws_getContext(orig_w, orig_h, orig_cp, new_w, new_h, new_cp, SWS_BILINEAR, NULL, NULL, NULL));

	pthread_rwlock_unlock(&resize_rwlock);

	//Check overlap
	if (active_streams_id.size()>1){
		overlap = check_overlap();
	}

#ifdef ENABLE_DEBUG
	cout << "New stream introduced" << endl;
	cout << "Id: " << id << endl;
	cout << "Original width: " << streams[id]->get_orig_w() << endl;
	cout << "Original height: " << streams[id]->get_orig_h() << endl;
	cout << "Original colorspace: " << streams[id]->get_orig_cp() << endl;
	cout << "Current width: " << streams[id]->get_curr_w() << endl;
	cout << "Current height: " << streams[id]->get_curr_h() << endl;
	cout << "Current colorspace: " << streams[id]->get_curr_cp() << endl;
	cout << "X Position: " << streams[id]->get_x_pos() << endl;
	cout << "Y Position: " << streams[id]->get_y_pos()<< endl;
	cout << "Layer: " << streams[id]->get_layer() << endl;
#endif


	return id;
}

int Layout::modify_stream (int stream_id, int width, int height, enum AVPixelFormat colorspace, int x_pos, int y_pos, int layer, bool keepAspectRatio){

	pthread_rwlock_wrlock(&resize_rwlock);

//TODO: resize rwlock podria ser un read lock si fem un wrlock d'un mutex intern de l'stream

	int id, old_x_pos = 0, old_y_pos = 0, old_width = 0, old_height = 0;
	//Check if id is active
	id = check_active_stream(stream_id);
	if (id==-1){
#ifdef ENABLE_DEBUG
		printf("Stream %d is not active", stream_id);
#endif
		pthread_rwlock_unlock(&resize_rwlock);
		return -1; //selected stream is not active
	}
	//Check if width, height, color space, xpos, ypos, layer are valid
	if (!check_modify_stream_values(width, height, colorspace, x_pos, y_pos, layer)){
#ifdef ENABLE_DEBUG
		printf("Introduced values are not valid.");
#endif
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}

	bool size_modified = false, pos_modified = false;

	//Modify stream features (the ones which have changed)
	if (width != -1){
		old_width = streams[id]->get_curr_w();
		streams[id]->set_curr_w(width);
		size_modified = true;
	}
	if (height != -1){
		old_height = streams[id]->get_curr_h();
		streams[id]->set_curr_h(height);
		size_modified = true;
	}
	if (colorspace != -1){
		streams[id]->set_curr_cp(colorspace);
	}
	if (x_pos != -1){
		old_x_pos = streams[id]->get_x_pos();
		streams[id]->set_x_pos(x_pos);
		pos_modified = true;
	}
	if (y_pos != -1){
		old_y_pos = streams[id]->get_y_pos();
		streams[id]->set_y_pos(y_pos);
		pos_modified = true;
	}
	if (layer != -1){
		streams[id]->set_layer(layer);
	}

	if (keepAspectRatio){
		//modify curr h in order to keep the same aspect ratio of orig_w and orig_h
		float orig_aspect_ratio;
		int delta;
		if (size_modified == false){
			old_width = streams[id]->get_curr_w();
			old_height = streams[id]->get_curr_h();
		}
		orig_aspect_ratio = streams[id]->get_orig_w()/streams[id]->get_orig_h();
		delta = floor((streams[id]->get_curr_w() - orig_aspect_ratio*streams[id]->get_curr_h())/orig_aspect_ratio);
		streams[id]->set_curr_h(streams[id]->get_curr_h() + delta);
		size_modified = true;
	}

	if (size_modified && !pos_modified){
		streams[id]->set_dummy_buffer((uint8_t*)realloc(streams[id]->get_dummy_buffer(), avpicture_get_size(streams[id]->get_curr_cp(), old_width, old_height) * sizeof(uint8_t)));
		avpicture_fill((AVPicture *)streams[id]->get_dummy_frame(), streams[id]->get_dummy_buffer(), streams[id]->get_curr_cp(), old_width, old_height);

		streams[id]->set_buffsize(avpicture_get_size(streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h()) * sizeof(uint8_t));
		streams[id]->set_buffer((uint8_t*)realloc(streams[id]->get_buffer(),*streams[id]->get_buffsize()));
		avpicture_fill((AVPicture *)streams[id]->get_current_frame(), streams[id]->get_buffer(), streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h());

		print_frame(streams[id]->get_x_pos(), streams[id]->get_y_pos(), old_width, old_height, streams[id]->get_dummy_frame(), layout_frame);

		streams[id]->set_dummy_buffer((uint8_t*)realloc(streams[id]->get_dummy_buffer(), *streams[id]->get_buffsize()));
		avpicture_fill((AVPicture *)streams[id]->get_dummy_frame(), streams[id]->get_dummy_buffer(), streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h());

		sws_freeContext (streams[id]->get_ctx());
		streams[id]->set_ctx(sws_getContext(streams[id]->get_orig_w(), streams[id]->get_orig_h(), streams[id]->get_orig_cp(),
				streams[id]->get_curr_w(), streams[id]->get_curr_h(), streams[id]->get_curr_cp(), SWS_BILINEAR, NULL, NULL, NULL));


	} else if (!size_modified && pos_modified){
		print_frame(old_x_pos, old_y_pos, streams[id]->get_curr_w(), streams[id]->get_curr_h(), streams[id]->get_dummy_frame(), layout_frame);

	} else if (size_modified && pos_modified){
		streams[id]->set_dummy_buffer((uint8_t*)realloc(streams[id]->get_dummy_buffer(), avpicture_get_size(streams[id]->get_curr_cp(), old_width, old_height) * sizeof(uint8_t)));
		avpicture_fill((AVPicture *)streams[id]->get_dummy_frame(), streams[id]->get_dummy_buffer(), streams[id]->get_curr_cp(), old_width, old_height);

		streams[id]->set_buffsize(avpicture_get_size(streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h()) * sizeof(uint8_t));
		streams[id]->set_buffer((uint8_t*)realloc(streams[id]->get_buffer(),*streams[id]->get_buffsize()));
		avpicture_fill((AVPicture *)streams[id]->get_current_frame(), streams[id]->get_buffer(), streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h());

		print_frame(old_x_pos, old_y_pos, old_width, old_height, streams[id]->get_dummy_frame(), layout_frame);

		streams[id]->set_dummy_buffer((uint8_t*)realloc(streams[id]->get_dummy_buffer(), *streams[id]->get_buffsize()));
		avpicture_fill((AVPicture *)streams[id]->get_dummy_frame(), streams[id]->get_dummy_buffer(), streams[id]->get_curr_cp(), streams[id]->get_curr_w(), streams[id]->get_curr_h());

		sws_freeContext (streams[id]->get_ctx());
		streams[id]->set_ctx(sws_getContext(streams[id]->get_orig_w(), streams[id]->get_orig_h(), streams[id]->get_orig_cp(),
				streams[id]->get_curr_w(), streams[id]->get_curr_h(), streams[id]->get_curr_cp(), SWS_BILINEAR, NULL, NULL, NULL));
	}
	
	pthread_rwlock_unlock(&resize_rwlock);
	

	pthread_rwlock_wrlock(streams[id]->get_needs_displaying_rwlock());
	streams[id]->set_needs_displaying(true);
	pthread_rwlock_unlock(streams[id]->get_needs_displaying_rwlock());

//	pthread_mutex_lock(streams[id]->get_orig_frame_ready_mutex());
//	streams[id]->set_orig_frame_ready(true);
//	pthread_cond_signal(streams[id]->get_orig_frame_ready_cond());
//	pthread_mutex_unlock(streams[id]->get_orig_frame_ready_mutex());

	//Check overlapping
	if (active_streams_id.size()>1){
		overlap = check_overlap();
	}

#ifdef ENABLE_DEBUG
	cout << "Stream " << id << " modified" << endl;
	cout << "Id: " << id << endl;
	cout << "Original width: " << streams[id]->get_orig_w() << endl;
	cout << "Original height: " << streams[id]->get_orig_h() << endl;
	cout << "Original colorspace: " << streams[id]->get_orig_cp() << endl;
	cout << "Current width: " << streams[id]->get_curr_w() << endl;
	cout << "Current height: " << streams[id]->get_curr_h() << endl;
	cout << "Current colorspace: " << streams[id]->get_curr_cp() << endl;
	cout << "X Position: " << streams[id]->get_x_pos() << endl;
	cout << "Y Position: " << streams[id]->get_y_pos()<< endl;
	cout << "Layer: " << streams[id]->get_layer() << endl;
#endif

	return 0;
}

int Layout::remove_stream (int stream_id){

	int id;
	//Check if id is active
	id = check_active_stream (stream_id);
	if (id==-1){
		return -1; //selected stream is not active
	}


	pthread_rwlock_wrlock(&resize_rwlock);
	print_frame(streams[id]->get_x_pos(), streams[id]->get_y_pos(), streams[id]->get_curr_w(), streams[id]->get_curr_h(), streams[id]->get_dummy_frame(), layout_frame);

	//Update stream id arrays
	for (i=0; i<=(int)active_streams_id.size(); i++){
		if (active_streams_id[i] == stream_id){
			free_streams_id.push_back(stream_id);
			active_streams_id.erase(active_streams_id.begin() + i);
		}
	}

	pthread_rwlock_unlock(&resize_rwlock);

	streams[id]->set_stream_to_default();

	//Check overlapping
	if (active_streams_id.size()>1){
		overlap = check_overlap();
	}

#ifdef ENABLE_DEBUG
	cout << "Stream " << id << " has been removed" << endl;
#endif

	return 0;
}

uint8_t* Layout::get_layout_bytestream(){
	pthread_rwlock_wrlock(&resize_rwlock);
	avpicture_layout((AVPicture *)layout_frame, lay_colorspace, lay_width, lay_height, out_buffer, lay_buffsize);
	pthread_rwlock_unlock(&resize_rwlock);
	return out_buffer;
}

int Layout::set_active(int stream_id, uint8_t active_flag){
	int id;
	id = check_active_stream (stream_id);
	if (id==-1){
		return -1; //selected stream is not active
	}

	pthread_rwlock_wrlock(&resize_rwlock);
	if (active_flag == 1){ //enable stream
		streams[id]->set_active(1);
		pthread_rwlock_unlock(&resize_rwlock);
		return 0;
	} else if (active_flag == 0){ //disable stream
		print_frame(streams[id]->get_x_pos(), streams[id]->get_y_pos(), streams[id]->get_curr_w(), streams[id]->get_curr_h(), streams[id]->get_dummy_frame(), layout_frame);
		streams[id]->set_active(0);
		pthread_rwlock_unlock(&resize_rwlock);
		return 0;
	} else {
		pthread_rwlock_unlock(&resize_rwlock);
		return -1;
	}
}

bool Layout::check_overlap(){

	pthread_rwlock_wrlock(&resize_rwlock);

	for (i=0; i<(int)active_streams_id.size(); i++){
		for (j=i+1; j<(int)active_streams_id.size(); j++){
			if(check_frame_overlap(
					streams[active_streams_id[i]]->get_x_pos(),
					streams[active_streams_id[i]]->get_y_pos(),
					streams[active_streams_id[i]]->get_curr_w(),
					streams[active_streams_id[i]]->get_curr_h(),
					streams[active_streams_id[j]]->get_x_pos(),
					streams[active_streams_id[j]]->get_y_pos(),
					streams[active_streams_id[j]]->get_curr_w(),
					streams[active_streams_id[j]]->get_curr_h()
				)){
				pthread_rwlock_unlock(&resize_rwlock);
				return true;
			}
		}
	}
	pthread_rwlock_unlock(&resize_rwlock);
	return false;
}

bool Layout::check_frame_overlap(int x1, int y1, int w1, int h1, int x2, int y2, int w2, int h2){
	if(x1<=x2 && x2<=x1+w1 && y1<=y2 && y2<=y1+h1){
		return true;
	}
	if(x1<=x2+w2 && x2+w2<=x1+w1 && y1<=y2 && y2<=y1+h1){
		return true;
	}
	if(x1<=x2 && x2<=x1+w1 && y1<=y2+h2 && y2+h2<=y1+h1){
		return true;
	}
	if(x1<=x2+w2 && x2+w2<=x1+w1 && y1<=y2+h2 && y2+h2<=y1+h1){
		return true;
	}
	if(x2<=x1 && x1<=x2+w2 && y2<=y1 && y1<=y2+h2){
		return true;
	}
	if(x2<=x1+w1 && x1+w1<=x2+w2 && y2<=y1 && y1<=y2+h2){
		return true;
	}
	if(x2<=x1 && x1<=x2+w2 && y2<=y1+h1 && y1+h1<=y2+h2){
		return true;
	}
	if(x2<=x1+w1 && x1+w1<=x2+w2 && y2<=y1+h1 && y1+h1<=y2+h2){
		return true;
	}

	return false;
}

int Layout::check_active_stream (int stream_id){

	int id;
	for (i=0; i<(int)active_streams_id.size(); i++){
		if(active_streams_id[i] == stream_id){
			id = active_streams_id[i];
			return id;
		}
	}
	return -1; //Stream is not active
}

int Layout::print_frame(int x_pos, int y_pos, int width, int height, AVFrame *stream_frame, AVFrame *layout_frame){

	int y, contTFrame, contSFrame, max_x, max_y, byte_init_point, byte_offset_line;
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

bool Layout::check_init_layout(int width, int height, enum AVPixelFormat colorspace, int max_streams){
	if (width <= 0 || height <= 0){
		return false;
	}
	if (max_streams <= 0 || max_streams > MAX_STREAMS){
		return false;
	}
	return true;
}

bool Layout::check_modify_stream_values(int width, int height, enum AVPixelFormat colorspace, int x_pos, int y_pos, int layer){
	if (width != -1 && (width<=0 || width>lay_width)){
		return false;
	}
	if (height != -1 && (height<=0 || height>lay_height)){
		return false;
	}
	if (x_pos != -1 && (x_pos<0 || x_pos>=lay_width)){
		return false;
	}
	if (y_pos != -1L && (y_pos<0 || y_pos>=lay_height)){
		return false;
	}
	if (layer != -1 && (layer<0 || layer>max_layers)){
		return false;
	}
	return true;
}

bool Layout::check_introduce_stream_values(int orig_w, int orig_h, enum AVPixelFormat orig_cp, int new_w, int new_h, enum  AVPixelFormat new_cp, int x, int y){
	if (orig_w <= 0 || orig_h <= 0){
		return false;
	}
	if (new_w <=0 || new_w > lay_width){
		return false;
	}
	if (new_h <=0 || new_h > lay_height){
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

bool Layout::check_introduce_frame_values (int width, int height, enum AVPixelFormat colorspace){
	if (width <= 0 || height <= 0){
		return false;
	}
	return true;
}

bool Layout::check_modify_layout (int width, int height, enum AVPixelFormat colorspace){
	if (width <= 0 || height <= 0){
		return false;
	}
	return true;
}

int Layout::get_w(){
	return lay_width;
}

int Layout::get_h(){
	return lay_height;
}

AVFrame* Layout::get_lay_frame(){
	return layout_frame;
}

Stream* Layout::get_stream(int stream_id){
	return streams[stream_id];
}

std::vector<int> Layout::get_streams_id(){
	return active_streams_id;
}

void Layout::print_active_stream_id(){
	uint c=0;
	printf("Active streams:\n");
	for (c=0; c<active_streams_id.size(); c++){
		printf("%d ", active_streams_id[c]);
	}
}

void Layout::print_free_stream_id(){
	uint c=0;
	printf("Free streams:");
	for (c=0; c<free_streams_id.size(); c++){
		printf("%d" , free_streams_id[c]);
	}
}

unsigned int Layout::get_buffsize(){
	return lay_buffsize;
}

int Layout::get_max_streams(){
	return max_streams;
}

void Layout::print_active_stream_info(){
	uint c=0;
	Stream *stream;

	for (c=0; c<active_streams_id.size(); c++){
		stream = streams[active_streams_id[c]];
		printf("Id: %d\n", stream->get_id());
		printf("Original width: %d\n", stream->get_orig_w());
		printf("Original height: %d\n", stream->get_orig_h());
		printf("Current width: %d\n", stream->get_curr_w());
		printf("Current height: %d\n", stream->get_curr_h());
		printf("X Position: %d\n", stream->get_x_pos());
		printf("Y Position: %d\n", stream->get_y_pos());
		printf("Layer: %d\n\n", stream->get_layer());
	}
}

Layout::~Layout(){
  	int i;
  	for (i=0; i<max_streams; i++){
  		printf("Pointer: %p ID: %d\n", streams[i]->get_thread(), streams[i]->get_id());
		pthread_cancel(streams[i]->get_thread());
  		delete streams[i];
  	}
  	avcodec_free_frame(&layout_frame);
  	free(lay_buffer);
  	free(out_buffer);
  	pthread_rwlock_destroy(&resize_rwlock);
	printf("Layout_destructor\n");
}
