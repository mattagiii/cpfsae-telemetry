OBJS = ./src/obj/acquire.o
CC = gcc
CFLAGS = -Wall -c
LFLAGS = -pthread -Wall

acquire : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o acquire

./src/obj/acquire.o : ./src/acquire.c ./src/acquire.h
	$(CC) $(CFLAGS) $(DEBUG) ./src/acquire.c -o $@

debug : DEBUG = -g

debug : acquire

.PHONY: clean

clean :
	rm -f ./src/obj/*.o acquire
