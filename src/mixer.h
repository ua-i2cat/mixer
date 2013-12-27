/*
 *  MIXER - A real-time video mixing application
 *  Copyright (C) 2013  Fundació i2CAT, Internet i Innovació digital a Catalunya
 *
 *  This file is part of thin MIXER.
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
 *			  Ignacio Contreras <ignacio.contreras@i2cat.net>
 */		

#ifndef MIXER_H_
#define MIXER_H_

#include <pthread.h>
#include <stdint.h>
#include <map>
#include <string>
#include <queue>
#include "layout.h"
#include "event.h"
#include "stat_manager.h"
extern "C"{
	#include <io_mngr/receiver.h>
	#include <io_mngr/transmitter.h>
}

using namespace std;

class Event;

class Mixer {
	public:
		struct Dst{
			uint32_t id; /**< Destination ID */
			char *ip; /**< Destination IP address */
			uint32_t port; /**< Destination port */
		};

		/**
        * Get an instance of Mixer classs
        * @return An instance of Mixer
        */

		/**
        * Get an instance of Mixer classs
        * @param layout_width Layout width
        * @param layout_height Layout height
        * @param in_port Listening port for input streams
        * @return An instance of Mixer
        */


		/**
        * Starts Mixer main routine and IO managers
        */
        void start();

		/**
        * Stops Mixer main routine and IO managers
        */
        void stop();

		/**
        * Add an input stream. Width and height are automatically detected
        * @return 1 if succeeded and 0 if not
        */
		void add_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

		/**
        * Removes an input stream
        * @param id Stream ID
        * @return 1 if succeeded and 0 if not
        */
		void remove_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);
		
		/**
		* Add a new crop to an input stream
		* @param id  Input stream id 
       	* @param crop_width cropping rectangle width in pixels 
       	* @param crop_height cropping rectangle height in pixels 
       	* @param crop_x cropping rectangle upper left corner x coordinate
       	* @param crop_y cropping rectangle upper left corner y coordinate
       	* @param rsz_width layout rectangle width in pixels
       	* @param rsz_height layout rectangle height in pixels
       	* @param rsz_x layout rectangle upper left corner x coordinate (dummy in case of output stream crops)
       	* @param rsz_y layout rectangle upper left corner y coordinate (dummy in case of output stream crops)
       	* @param layer layout rectangle layer (considering layer 1 image bottom)
       	*/
		void add_crop_to_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

		/**
        * Modify an input stream crop
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop
        * @param new_crop_width new cropping rectangle width in pixels 
       	* @param new_crop_height new cropping rectangle height in pixels
       	* @param new_crop_x new cropping rectangle upper left corner x coordinate
       	* @param new_crop_y new cropping rectangle upper left corner y coordinate
        * @return 1 if succeeded and 0 if not
        */
        void modify_crop_from_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Modify the layout rectangle associated to an input stream crop
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop
        * @param new_crop_width new cropping rectangle width in pixels 
       	* @param new_crop_height new cropping rectanble height in pixels
       	* @param new_crop_x new cropping rectangle upper left corner x coordinate
       	* @param new_crop_y new cropping rectangle upper left corner y coordinate
       	* @param new_layer new layout rectangle layer
        * @return 1 if succeeded and 0 if not
        */
        void modify_crop_resizing_from_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Remove a crop from an input stream
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop 
        * @return 1 if succeeded and 0 if not
        */
		void remove_crop_from_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

		/**
        * Add a new crop to the layout, creating a new output stream associated to it
        * @param crop_width cropping rectangle width in pixels 
       	* @param crop_height cropping rectangle height in pixels 
       	* @param crop_x cropping rectangle upper left corner x coordinate
       	* @param crop_y cropping rectangle upper left corner y coordinate
       	* @param output_width resized crop width
       	* @param output_height resized crop height
        * @return 1 if succeeded and 0 if not
        */
        void add_crop_to_layout(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Modify a crop from the layout
        * @param crop_id Id of the crop to be modified 
        * @param new_crop_width cropping rectangle width in pixels 
       	* @param new_crop_height cropping rectangle height in pixels 
       	* @param new_crop_x cropping rectangle upper left corner x coordinate
       	* @param new_crop_y cropping rectangle upper left corner y coordinate
        * @return 1 if succeeded and 0 if not
        */
        void modify_crop_from_layout(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Modify a crop from the layout
        * @param crop_id Id of the crop to be modified 
        * @param new_width resized crop width
       	* @param new_height resized crop height
        * @return 1 if succeeded and 0 if not
        */
        void modify_crop_resizing_from_layout(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Remove a crop from the layout and the output stream associated to it
        * @param crop_id Id of the crop to be removed 
        * @return 1 if succeeded and 0 if not
        */
        void remove_crop_from_layout(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Enable input stream crop displaying
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop ss
        * @return 1 if succeeded and 0 if not
        */
        void enable_crop_from_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Disable input stream crop displaying
        * @param stream_id Id of the stream that contains the crop 
        * @param crop_id Id of the crop ss
        * @return 1 if succeeded and 0 if not
        */
        void disable_crop_from_source(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        /**
        * Add a new destination associated to an output stream. 
        * @param ip Destination IP address 
        * @param port Destination port 
        * @param stream_id Output stream ID  
        * @return 1 if succeeded and 0 if not
        */
		void add_destination(Jzon::Object* rootNode, Jzon::Object *outRootNode);

		/**
        * Remove a destination 
        * @param id Destination ID 
        * @return 1 if succeeded and 0 if not
        */
		void remove_destination(Jzon::Object* rootNode, Jzon::Object *outRootNode);

        void get_streams(Jzon::Object* rootNode, Jzon::Object *outRootNode);
        void get_layout(Jzon::Object* rootNode, Jzon::Object *outRootNode);
        void get_stats(Jzon::Object* rootNode, Jzon::Object *outRootNode);
        void get_layout_size(Jzon::Object* rootNode, Jzon::Object* outRootNode);
        void get_stream(Jzon::Object* params, Jzon::Object* outRootNode);
        void get_crop_from_stream(Jzon::Object* params, Jzon::Object* outRootNode);

        /**
        * Get the destinations associated to an output stream
        * @param id Id of the output stream
        * @return A vector with the destinations associated to that stream
        */
        vector<Dst> get_output_stream_destinations(uint32_t id);


        void get_stats_maps(map<uint32_t,streamStats*> &input_stats, map<uint32_t,streamStats*> &output_stats);
        void push_event(Event e);
        Mixer(int width, int height, int in_port);
		~Mixer();

    private:
        pthread_t thread;
        receiver_t *receiver;
        transmitter_t *transmitter;
        stream_list *in_video_str;
        stream_list *out_video_str;
        stream_list *in_audio_str;
        stream_list *out_audio_str;
        Layout *layout;
        statManager *s_mng;
        bool should_stop;
        int max_frame_rate;
        uint32_t _in_port;
        uint8_t state;
        priority_queue<Event> eventQueue;
        pthread_mutex_t eventQueue_lock;

		void* main_routine(void);
		static void* execute_routine(void *context);
        int receive_frames();
        void update_input_frames();
        void update_output_frame_buffers();
        void update_output_frames();
        void initialize_action_mapping();

};



#endif /* MIXER_H_ */
