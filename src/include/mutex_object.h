/*
 * mutex_object.h
 *
 *  Created on: Jun 18, 2013
 *      Author: palau
 */

#ifndef MUTEX_OBJECT_H_
#define MUTEX_OBJECT_H_

#include <pthread.h>

class MutexObject {
private:
	pthread_mutex_t* mutex_;
public:
	MutexObject(pthread_mutex_t* mutex);
	~MutexObject();
};

#endif /* MUTEX_OBJECT_H_ */
