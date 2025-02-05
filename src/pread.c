/**
 *  Copyright (C) 2025 Masatoshi Fukunaga
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 */
#include <errno.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
// lua
#include <lauxhlib.h>
#include <lua_errno.h>

#if !defined(LUA_OK)
# define LUA_OK 0
#endif

/**
 * @brief pcall_pread_lua read data from file descriptor with pread syscall.
 * this function will push the following values to the stack.
 *  1. string or nil: read data or nil if failed.
 *  2. any: error message or nil if success or EAGAIN.
 *  3. boolean: true if EAGAIN.
 * @param L
 * @return int
 */
static int pcall_pread_lua(lua_State *L)
{
    int fd             = lua_tointeger(L, 1);
    lua_Integer nbyte  = lua_tointeger(L, 2);
    lua_Integer offset = lua_tointeger(L, 3);
    char *buf          = NULL;
    ssize_t n          = 0;
    luaL_Buffer b;

#if LUA_VERSION_NUM > 501
    buf = luaL_buffinitsize(L, &b, nbyte);
#else
    if (nbyte > LUAL_BUFFERSIZE) {
        // NOTE: in Lua 5.1, preparing buffer space API (luaL_prepbuffer) is not
        // available. so, use lua_newuserdata instead.
        buf = lua_newuserdata(L, nbyte);
    } else {
        // use luaL_Buffer to prepare buffer space if nbyte is less than
        // LUAL_BUFFERSIZE
        luaL_buffinit(L, &b);
        buf = luaL_prepbuffer(&b);
    }
#endif

RETRY:
    n = pread(fd, buf, nbyte, offset);
    if (n > 0) {
#if LUA_VERSION_NUM > 501
        luaL_pushresultsize(&b, n);
#else
        if (nbyte > LUAL_BUFFERSIZE) {
            lua_pushlstring(L, buf, n);
        } else {
            luaL_addsize(&b, n);
            luaL_pushresult(&b);
        }
#endif
        return 1;
    }

    if (n == 0 || errno == EAGAIN || errno == EWOULDBLOCK) {
        // reached to EOF or try again
        lua_settop(L, 0);
        lua_pushnil(L);
        lua_pushnil(L);
        lua_pushboolean(L, 1);
        return 3;
    } else if (errno == EINTR) {
        // retry if interrupted by signal
        goto RETRY;
    }

    // error occurred
    lua_settop(L, 0);
    lua_pushnil(L);
    lua_errno_new(L, errno, "pread");
    return 2;
}

static int pread_lua(lua_State *L)
{
    // if the first argument is integer, then treat as file descriptor.
    // otherwise, treat as file object and get file descriptor from it.
    int fd = (lauxh_isint(L, 1)) ? lauxh_checkint(L, 1) : lauxh_fileno(L, 1);
    lua_Integer nbyte  = lauxh_optinteger(L, 2, -1);
    lua_Integer offset = lauxh_optinteger(L, 3, -1);

    if (nbyte == 0) {
        // ignore zero
        lua_pushnil(L);
        return 1;
    }

    if (offset < 0) {
        // use current file offset if offset is not specified
        offset = lseek(fd, 0, SEEK_CUR);
        if (offset == -1) {
            lua_pushnil(L);
            lua_errno_new(L, errno, "lseek");
            return 2;
        }
    }

    if (nbyte < 0) {
        // read all data from the current offset to the end of file if nbyte is
        // not specified or negative value.
        struct stat st;
        if (fstat(fd, &st) == -1) {
            lua_pushnil(L);
            lua_errno_new(L, errno, "fstat");
            return 2;
        } else if (offset > st.st_size) {
            // reached to EOF
            lua_pushnil(L);
            lua_pushnil(L);
            lua_pushboolean(L, 1);
            return 3;
        }
        nbyte = st.st_size - offset;
    }

    // keep the file descriptor at the top of the stack to avoid GC
    lua_settop(L, 1);

    // call pcall_pread_lua function with pcall
    lua_pushcclosure(L, pcall_pread_lua, 0);
    lua_pushinteger(L, fd);
    lua_pushinteger(L, nbyte);
    lua_pushinteger(L, offset);
    switch (lua_pcall(L, 3, LUA_MULTRET, 0)) {
    case LUA_OK:
        // return values are already pushed to the stack
        return lua_gettop(L) - 1;

    default:
        // NOTE: In LuaJIT and Lua 5.3 does not return LUA_ERRMEM error if
        // memory allocation fails. so, it should check the error message to
        // determine the error type that is memory allocation error or not.
#if LUA_VERSION_NUM == 503
        if (!strstr(lua_tostring(L, -1), "not enough memory"))
#elif defined(LUA_LJDIR)
        if (!strstr(lua_tostring(L, -1), "length overflow"))
#endif
        {
            // otherwise, push nil and runtime error message
            lua_pushnil(L);
            lua_pushvalue(L, -2);
            lua_error_new(L, -1);
            return 2;
        }

    case LUA_ERRMEM:
        // memory allocation error
        lua_pushnil(L);
        lua_errno_new_with_message(L, ENOMEM, "pread", lua_tostring(L, -2));
        return 2;
    }
}

LUALIB_API int luaopen_io_pread(lua_State *L)
{
    lua_errno_loadlib(L);
    lua_pushcfunction(L, pread_lua);
    return 1;
}
