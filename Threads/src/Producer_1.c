/*
 * Producer_1.c
 *
 *  Created on: 10.05.2017
 *      Author: bs
 */
#include "Producer_1.h"

void producer_1(void) {
	for (int i = 0; i < LETTERS; i++) {
		lowCaseLetters[i] = i + ASCII_LOW_CASE;
	}
	while (TRUE) {
		int i = 0;
		while (i < LETTERS) {
		sem_wait(semSpace);
		pthread_mutex_lock(mutex);
		produce(lowCaseLetters[i]);
		pthread_mutex_unlock(mutex);
		sem_post(semEntries);
		sleep(3);
		i++;
		}
	}
}
