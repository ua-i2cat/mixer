#include "opencv2/highgui/highgui.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <pthread.h>

class Crop {

	private:
		uint32_t id, stream_id, dst_img_id;
		uint32_t crop_x, crop_y;
		uint32_t dst_x, dst_y;
		uint32_t layer;
		Mat *src_img;
		Mat crop_img;
		Size crop_img_sz, dst_img_sz;
		pthread_rwlock_t lock;
		pthread_rwlock_t *stream_lock;
		pthread_mutex_t *new_frame_lock;
		pthread_cond_t *new_frame_cond;
		uint8_t *new_frame;
		uint8_t run;
	    
	public:
        ~Crop();
        Crop(uint32_t crop_id, uint32_t crop_width, uint32_t crop_height, uint32_t crop_x, uint32_t crop_y, 
        	  uint32_t layer, uint32_t dst_width, uint32_t dst_height, uint32_t dst_x, uint32_t dst_y, Mat *stream_img_ref, 
        	   pthread_rwlock_t *str_lock_ref, pthread_cond_t *str_cond_ref, pthread_mutex_t *str_new_frame_lock_ref, uint8_t *str_new_frame_ref);
        int modify_crop(uint32_t new_crop_width, uint32_t new_crop_height, uint32_t new_crop_x, uint32_t new_crop_y);
        int modify_dst(uint32_t new_dst_width, uint32_t new_dst_height, uint32_t new_dst_x, uint32_t new_dst_y, uint32_t new_layer);
        int enable_dst();
        int disable_dst();

        void *resize(void);
		static void *execute_resize(void *context);

		uint32_t get_id();
		void set_id();

        resize(img(Rect(crop_x, crop_y, crop_width, crop_height)), crop_img, crop_img.size(), 0, 0, INTER_LINEAR);
       	crop_img.copyTo(dst_img(Rect(dst_x, dst_y, dst_width, dst_height)));

};