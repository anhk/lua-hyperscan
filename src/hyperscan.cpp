

#include "hs.h"

#include <vector>

#include <stdio.h>
#include <string.h>

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#define DEBUG_PRINT

#ifndef DEBUG_PRINT
#define dd(...)
#else
#define dd(fmt, ...) do {                           \
    fprintf(stderr, "[%s:%d] "fmt"\n",              \
            __FUNCTION__, __LINE__, ##__VA_ARGS__); \
} while (0)
#endif

#define MAGIC 0xFFEEAABB

typedef struct _hs_s {
    unsigned int magic;
    hs_database_t *db;
    hs_scratch_t *scratch;
} hs_t;

static int hyperscan_new(lua_State *L)
{
    hs_t *hs = (hs_t *) lua_newuserdata(L, sizeof(hs_t));

    hs->magic = MAGIC;
    hs->db = NULL;
    hs->scratch = NULL;

    luaL_getmetatable(L, "hyperscan");
    lua_setmetatable(L, -2);

    dd("malloc hs: %p", hs);
    return 1;
}

static int hyperscan_compile(lua_State *L)
{
    hs_t *hs = (hs_t *)luaL_checkudata(L, 1, "hyperscan");
    if (hs->magic != MAGIC) {
        return luaL_error(L, "bad input of object.");
    }
    if (hs->db != NULL) {
        return luaL_error(L, "hyperscan is already compiled.");
    }

    std::vector<const char *> patterns;
    std::vector<unsigned int> flags;
    std::vector<unsigned int> ids;

    lua_settop(L, 2);
    luaL_checktype(L, 2, LUA_TTABLE);

    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
        /**
         * -1=value, -2=key
         **/
        lua_pushvalue(L, -2);

        unsigned int key = lua_tonumber(L, -1);
        const char *value = strdup(lua_tostring(L, -2));
        if (value == NULL) {
            /** TODO: free all the memorys. **/
            return luaL_error(L, "no enough memory here.");
        }

        patterns.push_back(value);
        ids.push_back(key);
        flags.push_back(HS_FLAG_CASELESS|HS_FLAG_SOM_LEFTMOST);

        dd("%d: %s", key, value);
        lua_pop(L, 2);
    }
    lua_pop(L, 1);

    hs_compile_error_t *compileErr = NULL;
    hs_error_t err;

    err = hs_compile_multi(patterns.data(), flags.data(), ids.data(),
            patterns.size(),
            HS_MODE_BLOCK, NULL, &hs->db, &compileErr);

    dd("hs->db: %p", hs->db);

    for (size_t i = 0; i < patterns.size(); i ++) {
        free((char*)patterns[i]);
        patterns[i] = NULL;
    }

    if (err != HS_SUCCESS) {
        if (compileErr->expression < 0) {
            dd("ERROR: %s", compileErr->message);
        } else {
            dd("ERROR: Pattern '%s' failed compilation with error: %s",
                    patterns[compileErr->expression],
                    compileErr->message);
        }
        hs_free_compile_error(compileErr);
        return luaL_error(L, "compile patterns error.");
    }

    lua_pushboolean(L, 1);
    return 1;
}


static int onMatch(unsigned int id, unsigned long long from, unsigned long long to,
        unsigned int flags, void *ctx)
{
    dd("id=%d, from=%llu, to=%llu, flags=%x, ctx=%p",
            id, from, to, flags, ctx);
    std::vector<unsigned int> *matchResult = (std::vector<unsigned int>*)ctx;

    matchResult->push_back(id);
    return 0; // continue matching
}

static int hyperscan_match(lua_State *L)
{
    hs_error_t err;

    std::vector<unsigned int> matchResult;

    hs_t *hs = (hs_t *)luaL_checkudata(L, 1, "hyperscan");
    if (hs->magic != MAGIC) {
        return luaL_error(L, "bad input of object.");
    }
    if (hs->db == NULL) {
        return luaL_error(L, "hyperscan should be compiled.");
    }

    size_t len;
    const char *buff = luaL_checklstring(L, 2, &len);

    dd("buff: %s, len: %lu", buff, len);
    dd("match hs: %p, hs->db: %p", hs, hs->db);


    /** we call hs_alloc_scratch to
     *  let hyperscan decides whether hs->scratch should be freed **/
    err = hs_alloc_scratch(hs->db, &hs->scratch);

    if (err != HS_SUCCESS) {
        return luaL_error(L, "could not allocate scratch space.");
    }

    err = hs_scan(hs->db, buff, len, 0, hs->scratch, onMatch, &matchResult);
    if (err != HS_SUCCESS) {
        return luaL_error(L, "unable to scan.");
    }

    dd("match: %lu", matchResult.size());

    if (matchResult.size() <= 0) {
        lua_pushnil(L);
        lua_pushstring(L, "no match result.");
        return 2;
        //return luaL_error(L, "no match result.");
    }

    lua_createtable(L, matchResult.size(), 0);
    for (size_t i = 0; i < matchResult.size(); i ++) {
        lua_pushnumber(L, i+1);
        lua_pushinteger(L, matchResult[i]);
        lua_settable(L, -3);
    }

    return 1;
}

static int hyperscan_gc(lua_State *L)
{
    hs_t *hs = (hs_t *)luaL_checkudata(L, 1, "hyperscan");
    if (hs->magic != MAGIC) {
        return luaL_error(L, "bad input of object.");
    }

    if (hs->db == NULL) {
        return 0;
    }
    hs_free_database(hs->db);
    hs->db = NULL;
    dd("__gc hs: %p", hs);
    return 0;
}

static const struct luaL_Reg hyperscan_reg [] = {
    {"new", hyperscan_new},
    {"compile", hyperscan_compile},
    {"match", hyperscan_match},
    {"__gc", hyperscan_gc},
    {NULL, NULL}
};


extern "C" int luaopen_hyperscan(lua_State *L)
{
    luaL_newmetatable(L, "hyperscan");
#if LUA_VERSION_NUM >= 502
    luaL_setfuncs(L, hyperscan_reg, 0);
#else
    luaL_register(L, NULL, hyperscan_reg);
#endif

    lua_pushliteral(L, "__metatable");
    lua_pushliteral(L, "must not access this metatable");
    lua_settable(L, -3);

    lua_pushliteral(L, "__index");
    lua_pushvalue(L, -2);
    lua_rawset(L, -3);

    return 1;
}

