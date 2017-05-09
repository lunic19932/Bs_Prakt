/*
 * fifo.h
 *
 *  Created on: 09.05.2017
 *      Author: bs
 */

#ifndef SRC_FIFO_H_
#define SRC_FIFO_H_
#define FIFO_SIZE 10
#define FIFO_LastIndex FIFO_SIZE-1
#define NULL 0
#define TRUE 1
#define FALSE 0
#define LETTERS 26

extern sem_t semSpace, semEntries;
extern pthread_mutex_t mutex;

extern void fifo_init(void);


#endif /* SRC_FIFO_H_ */
