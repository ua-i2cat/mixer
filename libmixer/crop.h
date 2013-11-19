#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <pthread.h>
#include <stdint.h>

#ifndef TRUE
#define FALSE    0
#define TRUE    1
#endif /* TRUE */

using namespace cv;

/*! Crop class defines a crop from a reference stream. 
	It also defines position and size of the crop into the layout, in case of input stream crops. <br>
	For output stream crops, dst_x and dst_y (layout rectangle position) are not applicable */

class Crop {

	private:
		uint32_t id, stream_id, dst_img_id;
		uint32_t crop_x, crop_y;
		uint32_t dst_x, dst_y;
		uint32_t layer; 
		Mat src_cropped_img;
		Mat crop_img;
		Size crop_img_size, dst_img_size;
		pthread_rwlock_t lock;
		pthread_rwlock_t *stream_lock;
		uint8_t new_frame;
		uint8_t run;
		pthread_t thread;
		uint8_t active;
	    
        void *resize_routine(void);
		static void *execute_resize(void *context);
	
	public:
		/**
       	* Class constructor
       	* @param crop_id  crop id
       	* @param crop_width cropping rectangle width in pixels 
       	* @param crop_height cropping rectangle height in pixels 
       	* @param crop_x cropping rectangle upper left corner x coordinate
       	* @param crop_y cropping rectangle upper left corner y coordinate
       	* @param dst_width layout rectangle width in pixels
       	* @param dst_height layout rectangle height in pixels
       	* @param dst_x layout rectangle upper left corner x coordinate (dummy in case of output stream crops)
       	* @param dst_y layout rectangle upper left corner y coordinate (dummy in case of output stream crops)
       	* @param layer layout rectangle layer (considering layer 1 image bottom)
       	* @param stream_img_ref stream original image
       	* @param str_lock_ref stream_img_ref stream original image rwlock pointer
       	* @see Crop()
       	*/
        Crop(uint32_t crop_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, uint32_t layer, 
        		uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y, Mat stream_img_ref, pthread_rwlock_t *str_lock_ref); //!< Constructor.
        /**
       	* Modify origin stream crop size and position
       	* @param new_crop_width new cropping width in pixels 
       	* @param new_crop_height new cropping height in pixel 
       	* @param new_crop_x new cropping rectangle upper left corner x coordinate
       	* @param new_crop_y new cropping rectangle upper left corner y coordinate
       	* @param stream_img_ref stream original image
       	* @see Crop()
       	*/
        void modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y, Mat stream_img_ref);

        /**
       	* Modify layout rectangle size and positiion where the crop will be printed
       	* @param new_crop_width new layout rectangle width in pixels
       	* @param new_crop_height new layout rectangle height in pixels 
       	* @param new_crop_x new layout rectangle upper left corner x coordinate (dummy in case of output stream crops)
       	* @param new_crop_y new layout rectangle upper left corner y coordinate (dummy in case of output stream crops)
       	* @param stream_img_ref stream original image
       	* @see Crop()
       	*/
       	void modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y, uint32_t new_layer);

		uint32_t get_id();
		uint32_t get_layer();
		Mat get_crop_img();
		uint32_t get_crop_width();
		uint32_t get_crop_height();
		uint32_t get_crop_x();
		uint32_t get_crop_y();
		uint32_t get_dst_x();
		uint32_t get_dst_y();
		uint32_t get_dst_width();
		uint32_t get_dst_height();
		pthread_rwlock_t* get_lock();
		pthread_t get_thread();
		void stop();
		uint8_t is_active();
		uint8_t set_active(uint8_t act);
		uint8_t* get_buffer();
		uint32_t get_buffer_size();
		void set_new_frame(uint8_t n);
};