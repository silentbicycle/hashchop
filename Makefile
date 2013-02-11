CFLAGS += -std=c99 -Wall -g -O2 -fPIC

# These are only necessary if you build the Lua library.
LUA_LIBPATH=	/usr/local/lib/
LUA_INC=	/usr/local/include/
LUA_LIBS=	-llua -lm
LUA_FLAGS +=	-shared -fPIC
LUA_PROGNAME =  lua
LUA_LIBDEST=	/usr/local/lib/lua/5.1/

all: hashchop.o test bench

test: hashchop.o test.c
	${CC} -o $@ test.c hashchop.o ${CFLAGS} ${LDFLAGS}

lua: hashchop.so

test-lua: lua
	${LUA_PROGNAME} test.lua

bench: bench.c hashchop.o
	${CC} -o $@ bench.c hashchop.o ${CFLAGS} ${LDFLAGS}

hashchop.o: hashchop.c hashchop.h Makefile

hashchop.so: lhashchop.c hashchop.o hashchop.h Makefile
	${CC} -o hashchop.so lhashchop.c hashchop.o ${CFLAGS} \
		${LUA_FLAGS} -I ${LUA_INC} -L ${LUA_LIBPATH} ${LUA_LIBS}

lua-install: lua
	cp hashchop.so ${LUA_LIBDEST}

lua-uninstall:
	rm ${LUA_LIBDEST}/hashchop.so

TAGS:
	etags *.[ch]

clean:
	rm -f *.o *.a *.so TAGS test bench
