/*
 * Consumer_1.c
 *
 *  Created on: 10.05.2017
 *      Author: bs
 */
#include "Consumer.h"
consumer() {
	while (TRUE) {
			while (TRUE) {
			sem_wait(semEntries);
			pthread_mutex_lock(mutex);
			printf(consume());
			pthread_mutex_unlock(mutex);
			sem_post(semSpace);
			sleep(2);
			}
	}
}
