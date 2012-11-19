CFLAGS += -std=c99 -Wall -g -O2

all: hashchop.o test bench

test: hashchop.o test.c
	${CC} -o $@ test.c hashchop.o ${CFLAGS} ${LDFLAGS}

bench: bench.c hashchop.o
	${CC} -o $@ bench.c hashchop.o ${CFLAGS} ${LDFLAGS}

hashchop.o: hashchop.c hashchop.h Makefile

TAGS:
	etags *.[ch]

clean:
	rm -f *.o *.a *.so TAGS test bench
