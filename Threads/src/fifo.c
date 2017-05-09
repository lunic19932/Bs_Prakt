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

//void produce(char letter){
//
//	fifo[sem_getValue(semSpace)]=letter;
//
//
//}


