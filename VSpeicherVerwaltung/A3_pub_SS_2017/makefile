VMEM_PAGESIZE = 64

CC = gcc

CFLAGS = -g -std=gnu99 -pthread -Wall -DVMEM_PAGESIZE=$(VMEM_PAGESIZE)  -pedantic-errors 
LDFLAGS = -g -lrt -lpthread
SRC = mmanage.c vmappl.c vmaccess.c pagefile.c logger.c
OBJ = $(SRC:%.c=%.o)

all: mmanage vmappl
mmanage: mmanage.o pagefile.o logger.o
	$(CC) -o  mmanage $^ $(LDFLAGS)

vmappl: vmappl.o vmaccess.o
	$(CC) -o vmappl $^ $(LDFLAGS)

.PHONY: clean
clean:
	rm -rf $(OBJ)
	rm -rf mmanage vmappl
	rm -rf logfile.txt pagefile.bin

.PHONY: deps
deps:
	$(CC) -MM $(SRC) > makefile.dependencies

include makefile.dependencies
