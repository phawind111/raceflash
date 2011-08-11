CC=gcc
CFLAGS=-g -I./include -Wall
DEPS = ruf.h

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

raceflash: ruf.o raceflash.o ihex.o serial.o crc.o file.o flash.o 
	gcc -g -o raceflash ruf.o raceflash.o ihex.o serial.o crc.o file.o flash.o -I./include -lz

clean:	
	rm -f *.o
