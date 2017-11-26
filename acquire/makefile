# makefile
# makefile for acquire CAN frame acquisition service
#
# Created by: Matt Rounds
# Last updated: Nov 26, 2017

# set our compiler to gcc
CC = gcc
# flags for the compiler
CFLAGS = -Wall -c
# flags for the linker
LFLAGS = -pthread -Wall

OBJS = ./src/obj/acquire.o

# linking the final executable requires acquire.o and uses the linker flags
acquire : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o acquire

# compiling acquire.o requires source and header files
# uses the compiler flags and debug flag (if set), $@ is the name of the target
./src/obj/acquire.o : ./src/acquire.c ./src/acquire.h
	$(CC) $(CFLAGS) $(DEBUG) ./src/acquire.c -o $@

# if we run 'make debug', the debug flag will be set
debug : DEBUG = -g

# acauire is a dependency so that it is compiled with the debug flag and linked
debug : acquire

# 'make clean' doesn't depend on or create any files
.PHONY: clean

# 'make clean' deletes the executable and any object files
clean :
	rm -f ./src/obj/*.o acquire
