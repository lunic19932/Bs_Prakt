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
static sem_t *local_sem;
/**
 *****************************************************************************************
 *  @brief      This function setup the connection to virtual memory.
 *              The virtual memory has to be created by mmanage.c module.
 *
 *  @return     void
 ****************************************************************************************/
static void vmem_init(void) {
	key_t key = ftok(SHMKEY, SHMPROCID);
	int shm_id =shmget(key,SHMSIZE,0666|IPC_CREAT);
	vmem=shmat(shm_id,NULL,0);

	local_sem=sem_open(NAMED_SEM, O_CREAT | O_EXCL, 0644,0);
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

		if (vmem->pt.entries[address / VMEM_PAGESIZE].flags != PTF_PRESENT) {
			vmem->adm.req_pageno = address;
			kill(vmem->adm.mmanage_pid, SIGUSR1);

			sem_wait(local_sem);
			vmem->pt.entries[address / VMEM_PAGESIZE].age = 0x80;
		}
}

int vmem_read(int address) {
	if (vmem == NULL) {
		vmem_init();
	}
	vmem_put_page_into_mem(address);
	vmem->adm.g_count++;

	vmem->pt.entries[address / VMEM_PAGESIZE].count = vmem->adm.g_count;

//	vmem->pt.entries[address / VMEM_PAGESIZE].flags = PTHREAD_ TODO

	return vmem->data[vmem->pt.entries[address].frame * VMEM_PAGESIZE];
}

void vmem_write(int address, int data) {
	if (vmem == NULL) {
		vmem_init();
	}
	vmem_put_page_into_mem(address);
	vmem->adm.g_count++;
	vmem->pt.entries[address / VMEM_PAGESIZE].count = vmem->adm.g_count;
	vmem->data[vmem->pt.entries[address].frame * VMEM_PAGESIZE]=data;


}

// EOF
