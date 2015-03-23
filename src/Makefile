
#------------------------------------------------------------------------------
# File: Makefile
# 
# Note: This Makefile requires GNU make.
#------------------------------------------------------------------------------

CC = gcc
MCC = mpicc

OSTYPE = $(shell uname)

ifeq ($(OSTYPE),CYGWIN_NT-5.1)
ARCH = -D_CYGWIN_
endif

ifeq ($(OSTYPE),Linux)
ARCH = -D_LINUX_
endif

ifeq ($(OSTYPE),SunOS)
ARCH =  -D_SOLARIS_
endif

ifeq ($(OSTYPE),Darwin)
ARCH = -D_DARWIN_
endif

PATH_OUT = ./$(OSTYPE)/

CFLAGS = -Wall $(ARCH) -O3 -m64 
#LIBS = -lm -lpthread -lrt 
LIBS = -lpthread  

COM_OBJS = MapReduceScheduler.o

PROGS = mrsrf mrsrf-pq 

.PHONY: default clean

all: $(PROGS)

mrsrf-pq: $(COM_OBJS) mrsrf-pq.o mapreduce_sort.o
	$(CC) $(CFLAGS) -o $@ $(COM_OBJS) mrsrf-pq.o mapreduce_sort.o $(LIBS)

mrsrf: mrsrf-mpi.c  
	mpicc -o mrsrf mrsrf-mpi.c 

clean:
	rm -f $(PROGS) *.o *.a *~ *.tmp *.bak *.log mrsrf mrsrf-pq
