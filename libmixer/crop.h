#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <pthread.h>
#include <stdint.h>

#ifndef TRUE
#define FALSE    0
#define TRUE    1
#endif /* TRUE */

using namespace cv;

class Crop {

	private:
		uint32_t id, stream_id, dst_img_id;
		uint32_t crop_x, crop_y;
		uint32_t dst_x, dst_y;
		uint32_t layer;
		Mat src_img;
		Mat crop_img;
		Size crop_img_size, dst_img_size;
		pthread_rwlock_t lock;
		pthread_rwlock_t *stream_lock;
		pthread_mutex_t *new_frame_lock;
		pthread_cond_t *new_frame_cond;
		uint8_t *new_frame;
		uint8_t run;
		pthread_t thread;
		uint8_t active;
	    
	public:

		float diff;
		int diff_count;
		float diff_avg;

        Crop(uint32_t crop_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
        	  uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y, Mat stream_img_ref, 
        	   pthread_rwlock_t *str_lock_ref, pthread_cond_t *str_cond_ref, pthread_mutex_t *str_new_frame_lock_ref, uint8_t *str_new_frame_ref);
        void modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y);
       	void modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y, uint32_t new_layer);
        void enable_dst();
        void disable_dst();

        void *resize_routine(void);
		static void *execute_resize(void *context);

		uint32_t get_id();
		uint32_t get_layer();
		Mat get_crop_img();
		uint32_t get_dst_x();
		uint32_t get_dst_y();
		uint32_t get_dst_width();
		uint32_t get_dst_height();
		pthread_rwlock_t* get_lock();
		pthread_t get_thread();
		void stop();
		uint8_t is_active();
		uint8_t set_active(uint8_t act);



};