/*
 * fifo.c
 *
 *  Created on: 09.05.2017
 *      Author: bs
 */
#include "fifo.h"
#include <semaphore.h>
#include <pthread.h>

char fifo[FIFO_SIZE]={0};

void produce(char letter){
	for (int i = FIFO_LastIndex - sem_getValue(semEntries);
			i == FIFO_LastIndex; i++) {
		fifo[i] = fifo[i+1];
	}
	fifo[FIFO_LastIndex]=letter;
}

char consume() {
	char letter = fifo[sem_getValue(semSpace)];
	fifo[sem_getValue(semSpace)] = 0;
	return letter;
}


