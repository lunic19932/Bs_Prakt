/**
 * @file mmanage.c
 * @author Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * @date  2014

 * @brief Memory Manager module of TI BSP A3 virtual memory
 * 
 * This is the memory manager process that
 * works together with the vmaccess process to
 * manage virtual memory management.
 *
 * The memory manager process will be invoked
 * via a SIGUSR1 signal. It maintains the page table
 * and provides the data pages in shared memory.
 *
 * This process starts shared memory, so
 * it has to be started prior to the vmaccess process.
 *
 */

#include "mmanage.h"
#include "debug.h"
#include "pagefile.h"
#include "logger.h"
#include "vmem.h"
#include <pthread.h>
#include <semaphore.h>
/*
 * Signatures of private / static functions
 */

/**
 *****************************************************************************************
 *  @brief      This function fetchs a page out of the pagefile.
 *
 * It is mainly a wrapper of the corresponding function of module pagefile.c
 *
 *  @param      pt_idx Index of the page that should be fetched.
 * 
 *  @return     void 
 ****************************************************************************************/
static void fetch_page(int pt_idx);

/**
 *****************************************************************************************
 *  @brief      This function writes a page into the pagefile.
 *
 * It is mainly a wrapper of the corresponding function of module pagefile.c
 *
 *  @param      pt_idx Index of the page that should be written into the pagefile.
 * 
 *  @return     void 
 ****************************************************************************************/
static void store_page(int pt_idx);

/**
 *****************************************************************************************
 *  @brief      This function initializes the virtual memory.
 *
 *  In particular it creates the shared memory. The application just attachs to the
 *  shared memory.
 *
 *  @return     void 
 ****************************************************************************************/
static void vmem_init(void);

/**
 *****************************************************************************************
 *  @brief      This function finds an unused frame.
 *
 *  The framepage array of pagetable marks unused frames with VOID_IDX. 
 *  Based on this information find_free_frame searchs in vmem->pt.framepage for the 
 *  free frame with the smallest frame number.
 *
 *  @return     idx of the unused frame with the smallest idx. 
 *              If all frames are in use, VOID_IDX will be returned.
 ****************************************************************************************/
static int find_free_frame();

/**
 *****************************************************************************************
 *  @brief      This function update the page table for page vmem->adm.req_pageno.
 *              It will be stored in frame.
 *
 *  @param      frame The frame that stores the now allocated page vmem->adm.req_pageno.
 *
 *  @return     void 
 ****************************************************************************************/
static void update_pt(int frame);

/**
 *****************************************************************************************
 *  @brief      This function allocates a new page into memory. If all frames are in 
 *              use the corresponding page replacement algorithm will be called.
 *
 *  allocate_page gets the requested page via vmem->adm.req_pageno. Please take into
 *  account that allocate_page must update the page table and log the page fault 
 *  as well.
 *  allocate_page does all actions that must be down when the SIGUSR1 signal 
 *  indicates a page fault.
 *
 *  @return     void 
 ****************************************************************************************/
static void allocate_page(void);

/**
 *****************************************************************************************
 *  @brief      This function is the signal handler attached to system call sigaction
 *              for signal SIGUSR1, SIGUSR2 aund SIGINT.
 *
 * These three signals have the same signal handler. Based on the parameter signo the 
 * corresponding action will be started.
 *
 *  @param      signo Current signal that has be be handled.
 * 
 *  @return     void 
 ****************************************************************************************/
static void sighandler(int signo);

/**
 *****************************************************************************************
 *  @brief      This function dumps the page table to stderr.
 *
 *  @return     void 
 ****************************************************************************************/
static void dump_pt(void);

/**
 *****************************************************************************************
 *  @brief      This function implements page replacement algorithm aging.
 *
 *  @return     idx of the page that should be replaced.
 ****************************************************************************************/
static int find_remove_aging(void);

/**
 *****************************************************************************************
 *  @brief      This function implements page replacement algorithm fifo.
 *
 *  @return     idx of the page that should be replaced.
 ****************************************************************************************/
static int find_remove_fifo(void);

/**
 *****************************************************************************************
 *  @brief      This function implements page replacement algorithm clock.
 *
 *  @return     idx of the page that should be replaced.
 ****************************************************************************************/
static int find_remove_clock(void);

/**
 *****************************************************************************************
 *  @brief      This function selects and starts a page replacement algorithm.
 *
 *  It is just a wrapper for the three page replacement algorithms.
 *
 *  @return     The idx of the page that should be replaced.
 ****************************************************************************************/
static int find_remove_frame(void);

/**
 *****************************************************************************************
 *  @brief      This function cleans up when mmange runs out.
 *
 *  @return     void 
 ****************************************************************************************/
static void cleanup(void) ;

/**
 *****************************************************************************************
 *  @brief      This function scans all parameters of the porgram.
 *              The corresponding global variables vmem->adm.page_rep_algo will be set.
 * 
 *  @param      argc number of parameter 
 *
 *  @param      argv parameter list 
 *
 *  @return     void 
 ****************************************************************************************/
static void scan_params(int argc, char **argv);

/**
 *****************************************************************************************
 *  @brief      This function prints an error message and the usage information of 
 *              this program.
 *
 *  @param      err_str pointer to the error string that should be printed.
 *
 *  @return     void 
 ****************************************************************************************/
static void print_usage_info_and_exit(char *err_str);

/*
 * variables
 */

static struct vmem_struct *vmem = NULL; //!< Reference to shared memory
static int signal_number = 0;           //!< Number of signal received last
static sem_t *local_sem;                //!< OS-X Named semaphores will be stored locally due to pointer

int main(int argc, char **argv) {
    struct sigaction sigact;

    init_pagefile(); // init page file
    open_logger();   // open logfile

    /* Create shared memory and init vmem structure */
    vmem_init();
    TEST_AND_EXIT_ERRNO(!vmem, "Error initialising vmem");
    PRINT_DEBUG((stderr, "vmem successfully created\n"));

    // scan parameter 
    vmem->adm.program_name = argv[0];
    vmem->adm.page_rep_algo = VMEM_ALGO_FIFO;
    scan_params(argc, argv);

    /* Setup signal handler */
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    TEST_AND_EXIT_ERRNO(sigaction(SIGUSR1, &sigact, NULL) == -1, "Error installing signal handler for USR1");
    PRINT_DEBUG((stderr, "USR1 handler successfully installed\n"));

    TEST_AND_EXIT_ERRNO(sigaction(SIGUSR2, &sigact, NULL) == -1, "Error installing signal handler for USR2");
    PRINT_DEBUG((stderr,  "USR2 handler successfully installed\n"));

    TEST_AND_EXIT_ERRNO(sigaction(SIGINT, &sigact, NULL) == -1, "Error installing signal handler for INT");
    PRINT_DEBUG((stderr, "INT handler successfully installed\n"));

    /* Signal processing loop */
    while(1) {
        signal_number = 0;
        pause();
        if(signal_number == SIGUSR1) {  /* Page fault */
            PRINT_DEBUG((stderr, "Processed SIGUSR1\n"));
            signal_number = 0;
        }
        else if(signal_number == SIGUSR2) {     /* PT dump */
            PRINT_DEBUG((stderr, "Processed SIGUSR2\n"));
            signal_number = 0;
        }
        else if(signal_number == SIGINT) {
            PRINT_DEBUG((stderr, "Processed SIGINT\n"));
        }
    }

    return 0;
}

void scan_params(int argc, char **argv) {
    int i = 0;
    unsigned char param_ok = FALSE;

    // scan all parameters (argv[0] points to program name)
    if (argc > 2) print_usage_info_and_exit("Wrong number of parameters.\n");

    for (i = 1; i < argc; i++) {
        param_ok = FALSE;
        if (0 == strcasecmp("-fifo", argv[i])) {
            // page replacement strategies fifo selected 
            vmem->adm.page_rep_algo = VMEM_ALGO_FIFO;
            param_ok = TRUE;
        }
        if (0 == strcasecmp("-clock", argv[i])) {
            // page replacement strategies clock selected 
            vmem->adm.page_rep_algo = VMEM_ALGO_CLOCK;
            param_ok = TRUE;
        }
        if (0 == strcasecmp("-aging", argv[i])) {
            // page replacement strategies aging selected 
            vmem->adm.page_rep_algo = VMEM_ALGO_AGING;
            param_ok = TRUE;
        }
        if (!param_ok) print_usage_info_and_exit("Undefined parameter.\n"); // undefined parameter found
    } // for loop
}

void print_usage_info_and_exit(char *err_str) {
    fprintf(stderr, "Wrong parameter: %s\n", err_str);
    fprintf(stderr, "Usage : %s [OPTIONS]\n", vmem->adm.program_name);
    fprintf(stderr, " -fifo     : Fifo page replacement algorithm.\n");
    fprintf(stderr, " -clock    : Clock page replacement algorithm.\n");
    fprintf(stderr, " -aging    : Aging page replacement algorithm.\n");
    fprintf(stderr, " -pagesize=[8,16,32,64] : Page size.\n");
    fflush(stderr);
    exit(EXIT_FAILURE);
}

void sighandler(int signo) {
    signal_number = signo;
    if(signo == SIGUSR1) {
        allocate_page();
    } else if(signo == SIGUSR2) {
        dump_pt();
    } else if(signo == SIGINT) {
        cleanup();
        exit(EXIT_SUCCESS);
    }  
}

/* Your code goes here... */

void vmem_init(void) {
	key_t key=ftok(SHMKEY,SHMPROCID);
	int shm_id =shmget(key,SHMSIZE,0666|IPC_CREAT);
	vmem=shmat(shm_id,NULL,0);
	vmem->adm.mmanage_pid=getpid();
	vmem->adm.req_pageno=0;
	vmem->adm.next_alloc_idx=0;
	vmem->adm.pf_count=0;
	vmem->adm.g_count=0;

	local_sem=sem_open(NAMED_SEM, O_CREAT | O_EXCL, 0644,0);

	for(int i=0;i<VMEM_NPAGES;i++){
		vmem->pt.entries[i].age=0;
		vmem->pt.entries[i].count=0;
		vmem->pt.entries[i].flags=0;
		vmem->pt.entries[i].frame=VOID_IDX;
	}
	for(int i=0;i<VMEM_NFRAMES;i++){
			vmem->pt.framepage[i]=VOID_IDX;
		}



}



int find_free_frame() {
	for(int i=0;i<=VMEM_NFRAMES;i++){
		if(vmem->pt.framepage[i]==VOID_IDX){
			return i;
		}
	}
	return find_remove_frame();
}

void allocate_page(void) {
	int freeFrame = find_free_frame();
	int reqPage = vmem->adm.req_pageno;
	vmem->pt.framepage[freeFrame]=reqPage;
}

void fetch_page(int pt_idx) {
	int* frame=vmem->data+find_free_frame()*VMEM_PAGESIZE;
	fetch_page_from_pagefile(pt_idx, frame);
}

void store_page(int pt_idx) {
	int frame = vmem->pt.entries[pt_idx].frame;
	int* frameMem=vmem->data+vmem->pt.framepage[frame]*VMEM_PAGESIZE;
	 store_page_to_pagefile(pt_idx, frameMem);

}

void update_pt(int frame) {
	 int reqPage=vmem->adm.req_pageno;
	 vmem->pt.entries[reqPage].frame=frame;
	 vmem->pt.entries[reqPage].flags=PTF_PRESENT;
	 vmem->pt.framepage[frame]=reqPage;
}

int find_remove_frame(void) {
	int algo=vmem->adm.page_rep_algo;
	int freeFrame=0;
	if(algo==VMEM_ALGO_AGING){
		freeFrame=find_remove_aging();

	}
	if(algo==VMEM_ALGO_FIFO){
		freeFrame=find_remove_fifo();
	}
	if(algo==VMEM_ALGO_CLOCK){
		freeFrame=find_remove_clock();
	}
	vmem->
}

int find_remove_fifo(void) {
	static int counter = 0;
	vmem->pt.framepage[counter] = VOID_IDX;
	return counter++; //TODO UEBERPRUEFEN!!
}

int find_remove_aging(void) {
}

int find_remove_clock(void) {
}

void cleanup(void) {
}

void dump_pt(void) {
}
// EOF
