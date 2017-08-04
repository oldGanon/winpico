#include "pico.hpp"

static pico_state Pico = {};
static lua_State *PicoLua = {};
static pico_cart *PicoCart = {};

#include "math.cpp"
#include "graphics.cpp"
#include "coroutine.cpp"
#include "table.cpp"
#include "audio_xxx.cpp"

//
// INPUT
//

static i16
Pico_btn()
{
    i16 Controller = Pico.Hardware.Controllers[0] | 
                    (Pico.Hardware.Controllers[1] << 6);
    i16 Keyboard = Pico.Hardware.Keyboards[0] | 
                  (Pico.Hardware.Keyboards[1] << 6);
    return Controller | Keyboard;
}

static b8
Pico_btn(i16 i, i16 p)
{
    if (p < 0 || p > 1)
        return 0;
    b8 Controller = !!(Pico.Hardware.Controllers[p] & (1 << i));
    b8 Keyboard = !!(Pico.Hardware.Keyboards[p] & (1 << i));
    return Controller || Keyboard;
}

static int
Pico_btn(lua_State *L)
{
    i16 i = 0;
    i16 p = 0;
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        lua_pushnumber(L, INTtoFIX(Pico_btn()));
    }
    else if (args == 1)
    {
        i = lua_tonumber(L, 1).i;
        lua_pushboolean(L, Pico_btn(i, 0));
    }
    else
    {
        i = lua_tonumber(L, 1).i;
        p = lua_tonumber(L, 2).i;
        lua_pushboolean(L, Pico_btn(i, p));   
    }
   
    return 1;
}

//
// MAP
//

static u8
Pico_mget(i16 x, i16 y)
{
    if (x < 0 || y < 0 || x > 127 || y > 63)
        return 0;
    
    i32 i = y * 128 + x;
    if (i < 4096) 
        return Pico.Map[i];

    u8 MapValue = Pico.Gfx[i];
    return (MapValue >> 4) | (MapValue << 4);
}

static int
Pico_mget(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2) 
    {
        lua_pushnumber(L, {});
        return 1;
    }
    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    fixed MapValue = INTtoFIX(Pico_mget(x, y));
    lua_pushnumber(L, MapValue);
    return 1;
}

static void
Pico_mset(i16 x, i16 y, i16 v)
{
    if (x < 0 || y < 0 || x > 127 || y > 63 || v < 0 || v > 255) 
        return;
    
    i32 i = y * 128 + x;
    if (i < 4096) 
        Pico.Map[i] = (u8)v;
    else 
        Pico.Gfx[i] = (u8)((v >> 4) | (v << 4));
}

static int
Pico_mset(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 3) 
        return 0;
    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    i16 v = lua_tonumber(L, 3).i;
    Pico_mset(x, y, v);
    return 0;
}

static void
Pico_map(i16 celx, i16 cely, i16 sx, i16 sy, i16 celw, i16 celh, i16 layer = 0)
{
    if (layer < 0 || layer > 255) return;

    i16 dy = sy;
    for (i16 y = cely; y < cely + celh; ++y, dy += 8)
    {
        i16 dx = sx;
        for (i16 x = celx; x < celx + celw; ++x, dx += 8)
        {
            u8 MapValue = Pico_mget(x, y);
            if (!MapValue) 
                continue;

            if ((Pico_fget(MapValue) & layer) != layer)
                continue;

            Pico_spr(MapValue, dx, dy);            
        }
    }
}

static int
Pico_map(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 6) 
        return 0;
    
    i16 celx  = lua_tonumber(L, 1).i;
    i16 cely  = lua_tonumber(L, 2).i;
    i16 sx    = lua_tonumber(L, 3).i;
    i16 sy    = lua_tonumber(L, 4).i;
    i16 celw  = lua_tonumber(L, 5).i;
    i16 celh  = lua_tonumber(L, 6).i;
    i16 layer = (args > 6) ? lua_tonumber(L, 7).i : 0;
    Pico_map(celx, cely, sx, sy, celw, celh, layer);
    
    return 0;
}

//
// STRING
//

static int 
Pico_sub (lua_State *L)
{
    i32 args = lua_gettop(L);
    
    if (args < 2 || !lua_isstring(L, 1))
        return 0;

    mi l;
    const char *s = lua_tolstring(L, 1, &l);
    mi start = lua_tonumber(L, 2).i;
    mi end = (args > 2) ? lua_tonumber(L, 3).i : l;
    if (start < 1) start = 1;
    if (end > l) end = l;
    if (start <= end)
        lua_pushlstring(L, s + start - 1, end - start + 1);
    else 
        lua_pushliteral(L, "");
    return 1;
}

//
// RANDOM
//

struct random_state 
{
    u32 MT[624];
    i16 Index;
};

static random_state RNG;

static void 
Pico_srand(u32 seed)
{
    RNG.MT[0] = seed;

    for (i16 i = 1; i < 624; ++i)
        RNG.MT[i] = (1812433253 * (RNG.MT[i - 1] ^ (RNG.MT[i - 1] >> 30)) + i);

    RNG.Index = 624;
}

#define MASK_UPPER (1 << 31)
#define MASK_LOWER ((u32)MASK_UPPER - 1)
static u32
Pico_rndU32()
{
    if (RNG.Index >= 624)
    {
        for (i16 i = 0; i < 624; ++i)
        {
            u32 x = (RNG.MT[i] & MASK_UPPER);
            x += (RNG.MT[(i + 1) % 624] & MASK_LOWER);
            u32 xA = x >> 1;

            if (x & 1)
                xA ^= 0x9908B0DF;

            RNG.MT[i] = RNG.MT[(i + 397) % 624] ^ xA;
        }
        RNG.Index = 0;
    }

    u32 Result = RNG.MT[RNG.Index];
    Result ^= RNG.MT[RNG.Index] >> 11;
    Result ^= (Result >> 7) & 0x9D2C5680;
    Result ^= (Result >> 15) & 0xEFC60000;
    Result ^= (Result >> 18);
    ++RNG.Index;

    return Result;
}
#undef MASK_UPPER
#undef MASK_LOWER

static int
Pico_srand(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args > 0)
        Pico_srand((u32)lua_tonumber(L, 1).v);
    return 0;
}

static int
Pico_rnd(lua_State *L)
{
    r64 r = Pico_rndU32() * 2.328306435454494e-10;
    i32 args = lua_gettop(L);
    fixed one = INTtoFIX(1);
    fixed max = (args > 0) ? lua_tonumber(L, 1) : one;
    max.v = (u32)(r * (r64)(u32)max.v);
    lua_pushnumber(L, max);
    return 1;
}

//
// PICO
//

static int
Pico_stat(lua_State *L)
{
    fixed x = lua_tonumber(L, 1);
    switch (x.i)
    {
        case 0: 
        {
            // Memory usage
        } break;

        case 1: 
        {
            // CPU usage
        } break;

        case 4: 
        {
            // clipboard
        } break;

        case 16: 
        case 17:
        case 18:
        case 19:
        {
            // sfx 
        } break;

        case 20: 
        case 21:
        case 22:
        case 23:
        {
            // note
        } break;

        case 32: 
        {
            // mouse x
        } break;

        case 33: 
        {
            // mouse y
        } break;

        case 34: 
        {
            // mouse bitmask
        } break;
    }
    
    lua_pushnumber(L, {0});
    return 1;
}

#include "parse.cpp"

static void
Pico_ResetState()
{
    static_assert(sizeof(draw_state) == 64, "draw_state is not 64 bytes!");
    static_assert(sizeof(hardware_state) == 64, "hardware_state is not 64 bytes!");
    static_assert(sizeof(sound_effect) == 68, "sound_effect is not 68 bytes!");

    memcpy(&Pico, PicoCart, sizeof(pico_cart_data));

    Pico_clip();
    Pico_camera();
    Pico_pal();
    Pico_color();

    u64 Clock = __rdtsc();
    Pico_srand((Clock >> 32) ^ (Clock & (((u64)1 << 32) - 1)));
}

static void
Pico_LuaInit(lua_State *L)
{
    luaL_openlibs(L);
    
    // table
    lua_register(L, "all",   Pico_all);
    luaL_dostring(L, ForeachFunction);
    lua_register(L, "del",   Pico_del);
    lua_register(L, "add",   Pico_add);

    // graphics
    lua_register(L, "camera",   Pico_camera);
    lua_register(L, "circ",     Pico_circ);
    lua_register(L, "circfill", Pico_circfill);
    lua_register(L, "clip",     Pico_clip);
    lua_register(L, "cls",      Pico_cls);
    lua_register(L, "color",    Pico_color);
    lua_register(L, "cursor",   Pico_cursor);
    lua_register(L, "fget",     Pico_fget);
    lua_register(L, "flip",     Pico_flip);
    lua_register(L, "fset",     Pico_fset);
    lua_register(L, "line",     Pico_line);
    lua_register(L, "pal",      Pico_pal);
    lua_register(L, "palt",     Pico_palt);
    lua_register(L, "pget",     Pico_pget);
    lua_register(L, "print",    Pico_print);
    lua_register(L, "pset",     Pico_pset);
    lua_register(L, "rect",     Pico_rect);
    lua_register(L, "rectfill", Pico_rectfill);
    lua_register(L, "sget",     Pico_sget);
    lua_register(L, "spr",      Pico_spr);
    lua_register(L, "sset",     Pico_sset);
    lua_register(L, "sspr",     Pico_sspr);

    // coroutine
    lua_register(L, "cocreate", Pico_cocreate);
    lua_register(L, "coresume", Pico_coresume);
    lua_register(L, "costatus", Pico_costatus);
    lua_register(L, "yield",    Pico_yield);

    // string
    lua_register(L, "sub",      Pico_sub);

    // map
    lua_register(L, "mget",     Pico_mget);
    lua_register(L, "mset",     Pico_mset);
    lua_register(L, "map",      Pico_map);
    
    // input
    lua_register(L, "btn",      Pico_btn);
    
    // math
    lua_register(L, "band",     Pico_band);
    lua_register(L, "rnd",      Pico_rnd);
    lua_register(L, "flr",      Pico_flr);
    lua_register(L, "shl",      Pico_shl);
    lua_register(L, "abs",      Pico_abs);
    lua_register(L, "sgn",      Pico_sgn);
    lua_register(L, "sqrt",     Pico_sqrt);
    
    // ausio
    lua_register(L, "sfx",      Pico_sfx);
    
    // pico
    lua_register(L, "stat",     Pico_stat);

    // code
    if (luaL_loadbufferx(L, (char *)PicoCart->Code, PicoCart->CodeSize, CART_NAME, "b") != LUA_OK)
    {
        // error
        const char *ErrStr = lua_tostring(L, -1);
        lua_pop(L, 1);
        return;
    }

    // execute code
    if (lua_pcall(L, 0, 0, 0))
    {
        // error
        const char *ErrStr = lua_tostring(L, -1);
        lua_pop(L, 1);
        return;
    }

}

static void
Pico_Init()
{
    lua_State *L = luaL_newstate();
    Pico_ResetState();
    Pico_LuaInit(L);

    lua_getglobal(L, "_init");
    if(lua_pcall(L, 0, 0, 0) != 0)
    {
        const char *S = lua_tostring(L, -1);
        lua_pop(L,1);
    }

    PicoLua = L;
}

static void
Pico_Update()
{
    lua_State *L = PicoLua;
    lua_getglobal(L, "_update60");
    if(lua_pcall(L, 0, 0, 0) != 0)
    {
        const char *S = lua_tostring(L, -1);
        lua_pop(L,1);
    }
}

static void
Pico_Draw()
{
    lua_State *L = PicoLua;
    lua_getglobal(L, "_draw");
    if(lua_pcall(L, 0, 0, 0) != 0)
    {
        const char *S = lua_tostring(L, -1);
        lua_pop(L,1);
    }
}

static void
Pico_End()
{
    lua_close(PicoLua);
    PicoLua = 0;
}
