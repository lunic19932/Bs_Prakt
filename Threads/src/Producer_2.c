/*
 * Producer_2.c
 *
 *  Created on: 10.05.2017
 *      Author: bs
 */

#include "Producer_2.h"
void producer_1(void) {
	for (int i = 0; i < LETTERS; i++) {
		upperCaseLetters[i] = i + ASCII_UPPER_CASE;
	}
	while (TRUE) {
		int i = 0;
		while (i < LETTERS) {
		sem_wait(semEntries);
		pthread_mutex_lock(mutex);
		produce(upperCaseLetters[i]);
		pthread_mutex_unlock(mutex);
		sem_post(semEntries);
		sleep(3);
		i++;
		}
	}
}
