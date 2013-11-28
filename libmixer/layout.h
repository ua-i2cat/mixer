/*
 *  LIBMIXER - A video frame mixing library
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin LIBMIXER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors:  Marc Palau <marc.palau@i2cat.net>,
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

/*! Layout class is the main one. It defines the layout size and contains the list of input streams and the output stream. <br>
    It's is designed as an API so all the functions have to be called from outside (it doesn't have a main routine) */ 

class Stream;
class Crop;

class Layout {

    private:
        multimap<uint32_t, Crop*>::iterator layers_it;
        map<uint32_t, Stream*>::iterator streams_it;

        uint8_t check_values(uint32_t max_width, uint32_t max_height, uint32_t width, uint32_t height, uint32_t x, uint32_t y);

    protected:
        Stream *out_stream;                             //NOTE: Doxygen trick in order to show relationship between classes. There's 
        map<uint32_t, Stream*> streams;                 //      no inheritance between classes in the project so we can consider these 
        multimap<uint32_t, Crop*> crops_by_layers;      //      attributes as private.
          
    public:
        /**
        * Class constructor
        * @param width Width
        * @param height Height
        */
        Layout(uint32_t width, uint32_t height);

        /**
        * Add an input stream
        * @param stream_id Id
        * @param width Width
        * @param height Height
        * @return 1 if succeeded and 0 if not
        * @see Stream
        */
        int add_stream(uint32_t stream_id, uint32_t width, uint32_t height);

        int add_stream(uint32_t stream_id);

        int init_stream(uint32_t stream_id, uint32_t width, uint32_t height);

        /**
        * Get a pointer to the stream with the desired id
        * @param stream_id  Id of the input stream which we want the function to return
        * @return Pointer to the desired input stream (NULL if it's not in the input stream list)
        * @see Stream
        */
        Stream *get_stream_by_id(uint32_t stream_id);

        /**
        * Remove a stream and all its crops
        * @param stream_id  Id of the stream which we want to remove
        * @return 1 if succeeded and 0 if not
        * @see Stream
        */
        int remove_stream(uint32_t stream_id);

        /**
        * Introduce frame data to a stream.
        * @param stream_id Id of the stream we want to introduce data to
        * @param buffer Pointer to the data buffer which contains data to be introduced
        * @param buffer_length Buffer size
        * @see Stream::introduce_frame()
        */
        void introduce_frame_to_stream(uint32_t stream_id, uint8_t* buffer, uint32_t buffer_length);

        /**
        * Add a new crop to an input stream
        * @param stream_id Id of the stream we want to add the crop to
        * @return 1 if succeeded and 0 if not
        * @see Stream::add_crop()
        */
        int add_crop_to_stream(uint32_t stream_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
                    uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y);

        /**
        * Modify an input stream crop
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop
        * @return 1 if succeeded and 0 if not
        * @see Crop::modify_crop()
        */
        int modify_orig_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y);

        /**
        * Modify the layout rectangle associated to an input stream crop
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop 
        * @return 1 if succeeded and 0 if not
        * @see Crop::modify_dst()
        */
        int modify_dst_crop_from_stream(uint32_t stream_id, uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height,
                    uint32_t new_crop_x, uint32_t new_crop_y, uint32_t new_layer);

        /**
        * Remove a crop from an input stream
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop 
        * @return 1 if succeeded and 0 if not
        * @see Stream::remove_crop()
        */
        int remove_crop_from_stream(uint32_t stream_id, uint32_t crop_id);

        /**
        * Add a crop to the output stream (which is associated to the composed layout)
        * @return 1 if succeeded and 0 if not
        * @see Stream::add_crop()
        */
        int add_crop_to_output_stream(uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t rsz_width, uint32_t rsz_height);
        
        /**
        * Modify a crop from the output stream (which is associated to the composed layout)
        * @param crop_id Id of the crop to be modified 
        * @return 1 if succeeded and 0 if not
        * @see Crop::modify_crop()
        */
        int modify_crop_from_output_stream(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y);

        /**
        * Modify a crop resizing from the output stream (which is associated to the composed layout)
        * @param crop_id Id of the crop to be modified 
        * @return 1 if succeeded and 0 if not
        * @see Crop::modify_dst()
        */
        int modify_crop_resize_from_output_stream(uint32_t crop_id, uint32_t new_crop_width, uint32_t new_crop_height);

        /**
        * Remove a crop from the output stream (which is associated to the composed layout)
        * @param crop_id Id of the crop to be removed 
        * @return 1 if succeeded and 0 if not
        * @see Stream::remove_crop()
        */
        int remove_crop_from_output_stream(uint32_t crop_id);

        /**
        * Enable input stream crop displaying
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop ss
        * @return 1 if succeeded and 0 if not
        */
        int enable_crop_from_stream(uint32_t stream_id, uint32_t crop_id);

        /**
        * Disable input stream crop displaying
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop ss
        * @return 1 if succeeded and 0 if not
        */
        int disable_crop_from_stream(uint32_t stream_id, uint32_t crop_id);
        
        /**
        * Compose the layout using resized input stream crops. This method calls wake_up_crops().
        * @see Stream::wake_up_crops()
        */
        void compose_layout();

        /**
        * Check if stream id is associated to any of the input streams which are already in the layout
        * @return return 1 if succeeded and 0 if not
        */
        uint8_t check_if_stream_init(uint32_t stream_id);

        void resize_input_crops();
        void split_layout();
        int set_resized_output_buffer(uint32_t id, uint8_t *buffer);

        
        uint8_t* get_output_crop_buffer(uint32_t crop_id);
        uint32_t get_output_crop_buffer_size(uint32_t crop_id);

        Stream *get_out_stream();
        map<uint32_t, Stream*> get_streams();
        pthread_rwlock_t* get_streams_lock();

        uint8_t* get_buffer();
        uint32_t get_buffer_size();





};


#endif /* LAYOUT_H_ */
