//
// MATH
//

#define MIN(a,b) ((a<b)?a:b)
#define MAX(a,b) ((a>b)?a:b)
#define CLAMP(r,a,b) ((r<a)?a:(r>b)?b:r)

static int
Pico_band(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2) 
    {
        lua_pushnumber(L, {});
        return 1;
    }
    fixed x  = lua_tonumber(L, 1);
    fixed y  = lua_tonumber(L, 2);
    fixed r = {x.v & y.v};
    lua_pushnumber(L, r);
    return 1;
}

static int
Pico_sgn(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        lua_pushnumber(L, INTtoFIX(1));
        return 1;
    }

    fixed x  = lua_tonumber(L, 1);
    if (x.i < 0)
        lua_pushnumber(L, INTtoFIX(-1));
    else 
        lua_pushnumber(L, INTtoFIX(1));
    return 1;
}

static int
Pico_shl(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        lua_pushnumber(L, {});
    }
    else if (args == 1)
    {
        fixed num = lua_tonumber(L, 1);
        lua_pushnumber(L, num);
    }
    else
    {
        fixed num = lua_tonumber(L, 1);
        fixed bits = lua_tonumber(L, 2);
        fixed r = {num.v << bits.i};
        lua_pushnumber(L, r);
    }
    return 1;
}

static int
Pico_flr(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        lua_pushnumber(L, {});
    }
    else
    {
        fixed x  = lua_tonumber(L, 1);
        x.f = 0;
        if (x.i < 0)
            x.i += 1;
        lua_pushnumber(L, x);
    }
    return 1;
}

static int
Pico_abs(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        lua_pushnumber(L, {});
    }
    else
    {
        fixed x = lua_tonumber(L, 1);

        if (x.v < 0)
            x.v = -x.v;

        lua_pushnumber(L, x);        
    }
    return 1;
}

#include <math.h>

static int
Pico_sqrt(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        lua_pushnumber(L, {});
    }
    else
    {
        fixed x = lua_tonumber(L, 1);
        r64 r = sqrt((r64)x.v / FIXED_ONE);
        lua_pushnumber(L, {(i32)(r * FIXED_ONE)});
    }
    return 1;
}

static int
Pico_atan2(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2)
    {
        lua_pushnumber(L, {});
    }
    else
    {
        fixed x = lua_tonumber(L, 1);
        fixed y = lua_tonumber(L, 2);
        r64 r = atan2((r64)x.v / FIXED_ONE, (r64)x.v / FIXED_ONE);
        lua_pushnumber(L, {(i32)(r * FIXED_ONE)});
    }
    return 1;
}
