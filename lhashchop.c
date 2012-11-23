/* Lua wrapper for hashchop. */

#include <lua.h>
#include <lauxlib.h>
#include "hashchop.h"

static const char OK[] = "ok";
static const char UNDERFLOW[] = "underflow";
static const char OVERFLOW[] = "overflow";
static const char FULL[] = "full";

static const char *res_msgs[] = {FULL, OVERFLOW, UNDERFLOW, OK};
#define GET_STATUS(RES) (res_msgs[RES+3])

typedef struct {
    hashchop *h;
    size_t buf_sz;
    unsigned char buf[1];
} LHashchop;

#define CHECK_HC(N)                                                     \
    LHashchop *lh = luaL_checkudata(L, N, "Hashchop");                  \
    hashchop *hc = lh->h

/* Allocate a hashchopper object with its BITS argument on the stack.
 * Returns a hashchopper or (nil, "Bad bits argument.").*/
static int lhashchop_new(lua_State *L) {
    int bits = luaL_checkint(L, 1);
    size_t buf_sz = 1 << bits;
    LHashchop *lh = lua_newuserdata(L, sizeof(*lh) + buf_sz);
    if (lh == NULL) {
        /* FIXME If lua_newuserdata's alloc fails, does lua longjmp
         * into its normal error handling, or just return null? */
        lua_pushstring(L, "Hashchop alloc failure");
        lua_error(L);
    }
    hashchop *hc = hashchop_new(bits);

    if (hc == NULL) {
        lua_pop(L, 1);
        lua_pushnil(L);
        lua_pushstring(L, "Bad bits argument.");
        return 2;
    } else {
        lh->h = hc;
        lh->buf_sz = buf_sz;
        luaL_getmetatable(L, "Hashchop");
        lua_setmetatable(L, -2);
        return 1;
    }
}

/* Sink a binary string into the hashchopper.
 * Returns a result string. */
static int lhashchop_sink(lua_State *L) {
    CHECK_HC(1);
    size_t sz = 0;
    hashchop_res res = HASHCHOP_OK;
    const char *str = lua_tolstring(L, 2, &sz);
    res = hashchop_sink(hc, (unsigned char *)str, sz);
    lua_pushstring(L, GET_STATUS(res));
    return 1;
}

/* Poll for a complete chunk from the hashchopper.
 * Returns the chunk string, or (nil, error code). */
static int lhashchop_poll(lua_State *L) {
    CHECK_HC(1);
    size_t sz = 0;
    hashchop_res res = hashchop_poll(hc, lh->buf, &sz);
    if (res == HASHCHOP_OK) {
        lua_pushlstring(L, (char *)lh->buf, sz);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, GET_STATUS(res));
        return 2;
    }
}

/* Get the remaining data from the hashchopper and reset it.
 * Returns the last chunk or (nil, error code). */
static int lhashchop_finish(lua_State *L) {
    CHECK_HC(1);
    size_t sz = lh->buf_sz;
    hashchop_res res = hashchop_finish(hc, lh->buf, &sz);
    if (res == HASHCHOP_OK) {
        lua_pushlstring(L, (char *)lh->buf, sz);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushstring(L, GET_STATUS(res));
        return 2;
    }
}

/* Reset the hashchopper. */
static int lhashchop_reset(lua_State *L) {
    CHECK_HC(1);
    hashchop_reset(hc);
    return 0;
}

static int lhashchop_gc(lua_State *L) {
    CHECK_HC(1);
    hashchop_free(hc);
    return 0;
}

static int lhashchop_tostring(lua_State *L) {
    #define BUF_SZ 24
    char buf[BUF_SZ];
    LHashchop *lh = luaL_checkudata(L, 1, "Hashchop");
    if (BUF_SZ < snprintf(buf, BUF_SZ - 1, "hashchop: %p", lh)) {
        lua_pushstring(L, "(snprintf error in tostring)");
    } else {
        lua_pushstring(L, buf);
    }
    return 1;
}

/* Library's table, containing only hashchop.new. */
static const struct luaL_Reg lhashchop_lib[] = {
    { "new", lhashchop_new },
    { NULL, NULL },
};

/* Metatable for all hashchoppers. */
static const struct luaL_Reg lhashchop_mt[] = {
    { "sink", lhashchop_sink },
    { "poll", lhashchop_poll },
    { "finish", lhashchop_finish },
    { "reset", lhashchop_reset },
    { "__gc", lhashchop_gc },
    { "__tostring", lhashchop_tostring },
    { NULL, NULL },
};

int luaopen_hashchop(lua_State *L) {
    /* Set up Hashchop metatable */
    luaL_newmetatable(L, "Hashchop");
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_register(L, NULL, lhashchop_mt);
    lua_pop(L, 1);

    luaL_register(L, "hashchop", lhashchop_lib);
    return 1;
}
