/*
 * Stream.h
 *
 *  Created on: Jun 13, 2013
 *      Author: palau
 */

#ifndef STREAM_H_
#define STREAM_H_

class Crop;

class Stream {

	private:
		uint32_t id, width, height;
		Mat img;
		std::map<uint32_t, Crop*> crops;

	public:
		Stream(uint32_t stream_id, uint32_t stream_width, uint32_t stream_height); 
		~Stream();
		

		void *resize(void);
		static void *execute_resize(void *context);

};



#endif /* STREAM_H_ */
