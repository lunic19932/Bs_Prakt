/**
 * @file vmaccess.c
 * @author Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * @date 2010
 * @brief The access functions to virtual memory.
 */

#include "vmem.h"
#include "debug.h"

/*
 * static variables
 */

static struct vmem_struct *vmem = NULL; //!< Reference to virtual memory
static sem_t *local_sem = NULL;
/**
 *****************************************************************************************
 *  @brief      This function setup the connection to virtual memory.
 *              The virtual memory has to be created by mmanage.c module.
 *
 *  @return     void
 ****************************************************************************************/
static void vmem_init(void) {
	key_t key = ftok(SHMKEY, SHMPROCID);
	int shm_id =shmget(key,SHMSIZE,0666);
	if (shm_id == -1) {
			perror("Error connecting VMEM from VMACCESS");
		}
	vmem=shmat(shm_id,NULL,0);

    local_sem = sem_open(NAMED_SEM, 0);
	if (SEM_FAILED == local_sem) {
		    perror("Fehler in sem_open: ");
		}

}

/**
 *****************************************************************************************
 *  @brief      This function does aging for aging page replacement algorithm.
 *              It will be called periodic based on g_count.
 *              This function must be used only when aging page replacement algorithm is activ.
 *              Otherwise update_age_reset_ref may interfere with other page replacement 
 *              alogrithms that base on PTF_REF bit.
 *
 *  @return     void
 ****************************************************************************************/
static void update_age_reset_ref(void) {
	int page;
	for (int i = 0; i < VMEM_NFRAMES; i++) {
		page = vmem->pt.framepage[i];
		if (vmem->pt.framepage[i] != VOID_IDX) {
			vmem->pt.entries[page].age >>= 1;

			if ((vmem->pt.entries[page].flags & PTF_REF) == PTF_REF) {
				vmem->pt.entries[page].age |= 0x80;
				vmem->pt.entries[page].flags &= ~PTF_REF;
			}
		}
	}

}

/**
 *****************************************************************************************
 *  @brief      This function puts a page into memory (if required).
 *              It must be called by vmem_read and vmem_write
 *
 *  @param      address The page that stores the contents of this address will be put in (if required).
 * 
 *  @return     void
 ****************************************************************************************/
static void vmem_put_page_into_mem(int address) {
		int page = address / VMEM_PAGESIZE;
		if (page < 0 || page >= VMEM_NPAGES) {
				perror("Index out of bounds!");
				kill(vmem->adm.mmanage_pid, SIGUSR2);
				key_t key = ftok(SHMKEY, SHMPROCID);
				int shmid = shmget(key, SHMSIZE, 0666);
				shmctl(shmid, IPC_RMID, NULL);
				vmem = NULL;
				exit(EXIT_FAILURE);
			}

		if ((vmem->pt.entries[page].flags & PTF_PRESENT) == 0) {
			vmem->adm.req_pageno = page;
			kill(vmem->adm.mmanage_pid, SIGUSR1);

			sem_wait(local_sem);
			vmem->pt.entries[page].age = 0x80;
		}
}

int vmem_read(int address) {
	if (vmem == NULL) {
		vmem_init();
	}
	vmem_put_page_into_mem(address);
	vmem->adm.g_count++;
	int page = address / VMEM_PAGESIZE;
	int data_offset = address - page * VMEM_PAGESIZE;
	int frame_offset = vmem-> pt.entries[page].frame * VMEM_PAGESIZE;

	vmem->pt.entries[page].count = vmem->adm.g_count;
	vmem->pt.entries[page].flags |= PTF_REF;

	if((vmem->adm.page_rep_algo == VMEM_ALGO_AGING) && ((vmem->adm.g_count % UPDATE_AGE_COUNT) == 0)){
	update_age_reset_ref();
	}
	return vmem->data[frame_offset + data_offset];
}

void vmem_write(int address, int data) {
	if (vmem == NULL) {
		vmem_init();
	}
	int page = address / VMEM_PAGESIZE;
	vmem_put_page_into_mem(address);
	vmem->adm.g_count++;
	vmem->pt.entries[page].count = vmem->adm.g_count;
	vmem->pt.entries[page].flags |= PTF_REF;

	int data_offset = address - page * VMEM_PAGESIZE;
	int frame_offset = vmem->pt.entries[page].frame * VMEM_PAGESIZE;

	vmem->data[frame_offset + data_offset]=data;
	vmem->pt.entries[page].flags |= PTF_DIRTY;

	if((vmem->adm.page_rep_algo == VMEM_ALGO_AGING) && ((vmem->adm.g_count % UPDATE_AGE_COUNT) == 0)){
		update_age_reset_ref();
		}
}

// EOF
