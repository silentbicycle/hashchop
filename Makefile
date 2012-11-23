CFLAGS += -std=c99 -Wall -g -O2

# These are only necessary if you build the Lua library.
LUA_LIBPATH=	-L/usr/local/lib/
LUA_INC=	-I/usr/local/include/
LUA_LIBS=	-llua -lm
LUA_FLAGS +=	-shared -fPIC

all: hashchop.o test bench

test: hashchop.o test.c
	${CC} -o $@ test.c hashchop.o ${CFLAGS} ${LDFLAGS}

test-lua: hashchop.so
	lua test.lua

bench: bench.c hashchop.o
	${CC} -o $@ bench.c hashchop.o ${CFLAGS} ${LDFLAGS}

hashchop.o: hashchop.c hashchop.h Makefile

hashchop.so: lhashchop.c hashchop.o hashchop.h Makefile
	${CC} -o hashchop.so lhashchop.c hashchop.o ${CFLAGS} ${LUA_FLAGS} ${LUA_INC} ${LUA_LIBS}

TAGS:
	etags *.[ch]

clean:
	rm -f *.o *.a *.so TAGS test bench
