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
 
#ifndef STREAM_H_
#define STREAM_H_

#include "crop.h"
#include <map>

using namespace cv;
using namespace std;


/*! Stream class defines a whole stream. It basically contains its width and height and a list of its crops. */ 
class Crop;

class Stream {

	private:
		uint32_t id;
		Size sz;
		Mat img;
              uint8_t new_frame;
              map<uint32_t, Crop*>::iterator it;

       protected:
		map<uint32_t, Crop*> crops;      //NOTE: Doxygen trick in order to show relationship between classes. There's 
                                               //      no inheritance between classes in the project so we can consider this 
                                               //      attribute as private.

	public:
		/**
       	* Class constructor
       	* @param stream_id Id
       	* @param stream_width Width
       	* @param stream_height Height
       	*/
		Stream(uint32_t stream_id, uint32_t stream_width, uint32_t stream_height); 

              /**
              * Class constructor without setting width and height. It's necessary to apply init(width,height) to this stream.
              * @param stream_id Id
              */
              Stream(uint32_t stream_id);

              /**
              * Set width and height for stream created with the constructor Stream(uint32_t stream_id) 
              * @param stream_id Id
              */
              void init(uint32_t stream_width, uint32_t stream_height);

              /**
              * Add a crop to the stream
              * @param id  Id
              * @param crop_width see Crop 
              * @param crop_height see Crop
              * @param crop_x see Crop 
              * @param crop_y see Crop 
              * @param layer see Crop 
              * @param dst_width see Crop 
              * @param dst_height see Crop 
              * @param dst_x see Crop 
              * @param dst_y see Crop 
              * @return Pointer to the added crop
              */

		Crop* add_crop(uint32_t id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y,
				uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y);

              int add_crop(uint32_t id);

		/**
       	* Get a pointer to the crop with the desired id
       	* @param crop_id  Id of the crop which we want the function to return
       	* @return Pointer to the desired crop (NULL if it's not in the stream crop list)
       	* @see Crop::Crop()
       	*/
		Crop* get_crop_by_id(uint32_t crop_id);

		/**
       	* Remove a crop from the stream
       	* @param crop_id  Id of the crop which we want to remove
       	* @return 1 if succeeded and 0 if not
       	* @see Crop
       	*/
		int remove_crop(uint32_t crop_id);

		/**
       	* Introduce frame data to the stream. This method calls wake_up_crops().
       	* @param buffer Pointer to the data buffer which contains data to be introduced
       	* @param buffer_length Buffer size
       	* @see wake_up_crops()
       	*/
		void introduce_frame(uint8_t* buffer, uint32_t buffer_length);

		/**
       	* Creates resize routines of the crops associated to the stream.
       	* @see Crop::resize_routine()
       	*/
              void create_resize_crops_routine();

              /**
              * Waits resize routines of the stream crops until they finish
              * @see Crop::resize_routine()
              */
              void wait_for_resize_crops_routine();

              /**
              * Waits resize routines of the stream crops until they finish
              * @see Crop::resize_routine()
              */
              void resize_crops();

              /**
              * Sets stream img to a rgb colour
              * @param r 0-255 red valuee 
              * @param g 0-255 green value
              * @param b 0-255 blue value
              * @see Layout::compose_layout()
              */
              void set_img_colour(int b, int g, int r);

              uint8_t has_new_frame();
              void set_new_frame(uint8_t new_flag);
              uint32_t get_id();
              map<uint32_t, Crop*>* get_crops();
              Mat get_img();
              uint32_t get_width();
              uint32_t get_height();

};



#endif /* STREAM_H_ */
