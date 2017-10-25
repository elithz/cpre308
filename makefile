#makefile to ensure Project2 is up to date
#usage: simply type "make" to compile all files that you need recompiled
#use make clean to remove .o and executable files.

#compiler to use
CC=gcc

ALL=baMng

all: $(ALL)

#executable
baMng: baMng.o Bank.o
	$(CC) -pthread -g -o baMng baMng.o Bank.o

#object files
baMng.o: baMng.c
	$(CC) -g -c baMng.c
Bank.o: Bank.c
	$(CC) -g -c Bank.c

#cleanup files
clean:
	rm -f $(ALL) *.o