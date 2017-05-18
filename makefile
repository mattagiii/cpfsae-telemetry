OBJS = ./src/obj/acquire.o
CC = gcc
CFLAGS = -Wall -c
LFLAGS = -pthread -Wall

acquire : $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o acquire

./src/obj/acquire.o : ./src/acquire.c ./src/acquire.h
	$(CC) $(CFLAGS) ./src/acquire.c -o $@

.PHONY: clean

clean :
	rm -f ./src/obj/*.o acquire
