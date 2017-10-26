

#usage: type "make" to compile all files
#use make clean to remove .o and executable files.

#compiler to use
CC=gcc

ALL=baMng baMng_coarse

all: $(ALL)

#executable
baMng: baMng.o Bank.o
	$(CC) -pthread -g -o baMng baMng.o Bank.o
baMng_coarse: baMng_coarse.o Bank.o
	$(CC) -pthread -g -o baMng baMng.o Bank.o

#object files
baMng.o: baMng.c
	$(CC) -g -c baMng.c
baMng.o: baMng_coarse.c
	$(CC) -g -c baMng_coarse.c
Bank.o: Bank.c
	$(CC) -g -c Bank.c

#cleanup files
clean:
	rm -f $(ALL) *.o