/*
 * mutex_object.cpp
 *
 *  Created on: Jun 18, 2013
 *      Author: palau
 */

#include "mutex_object.h"

MutexObject::MutexObject(pthread_mutex_t* mutex) {
	pthread_mutex_t* mutex_ = mutex;
	pthread_mutex_lock(mutex_);

}

MutexObject::~MutexObject() {
	pthread_mutex_unlock(mutex_);
}

