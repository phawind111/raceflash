CC=gcc
CFLAGS=
DEPS = 

%.o: %.c $(DEPS)
	$(CC) -g -c -o $@ $< $(CFLAGS)

ruf_extract: ruf_extract.o
	gcc -o ruf_extract ruf_extract.o  -lz

ruf_combine: ruf_combine.o
	gcc -o ruf_combine ruf_combine.o  -lz

all: ruf_combine ruf_extract

clean:	
	rm -f *.o
	rm ruf_extract
	rm ruf_combine
