#!/bin/sh

CC = gcc
CFLAGS =  -Wall -g -O0 -Wwrite-strings -Wshadow -pedantic-errors -fstack-protector-all

# On Mac, you may need to remove -ansi

all:  asm sim sim-pipe

asm:  asm.c
	$(CC) $(CFLAGS) -c asm.c
	$(CC) $(CFLAGS) -o asm asm.o
	chmod u+x asm

sim:  mips-small.c
	$(CC) $(CFLAGS) -c mips-small.c
	$(CC) $(CFLAGS) -o sim mips-small.o
	chmod u+x sim


sim-pipe:  mips-small-pipe.c mips-small-pipe.h
	$(CC) $(CFLAGS) -c mips-small-pipe.c
	$(CC) $(CFLAGS) -o sim-pipe mips-small-pipe.o
	chmod u+x sim-pipe

clean:  
	$(RM) *.o asm sim sim-pipe
