//
// GRAPHICS
//

static u8
Pico_pget(i16 x, i16 y)
{
    if (x < 0 || x > 127) return 0;
    if (y < 0 || y > 127) return 0;
    return Pico_PackedGet(Pico.Screen, y * 128 + x);
}

static int
Pico_pget(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2) 
        return 0;

    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    lua_pushnumber(L, INTtoFIX(Pico_pget(x, y)));
    return 1;
}

static void
Pico_pset(i16 x, i16 y, i16 col)
{
    x -= Pico.Draw.CameraX;
    y -= Pico.Draw.CameraY;

    if (x < Pico.Draw.ClipX0) return;
    if (y < Pico.Draw.ClipY0) return;
    if (x >= Pico.Draw.ClipX1) return;
    if (y >= Pico.Draw.ClipY1) return;

    if (col > 15 || col < 0) col = 0;
    col = Pico.Draw.PaletteDraw[(u8)col] & 15;
    Pico_PackedSet(Pico.Screen, y * 128 + x, (u8)col);
}

static int
Pico_pset(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2) 
        return 0;

    i16 col = Pico.Draw.Color;
    switch (args)
    {
        default: col = lua_tonumber(L, 3).i;
        case 2:
        {
            i16 x = lua_tonumber(L, 1).i;
            i16 y = lua_tonumber(L, 2).i;
            Pico_pset(x, y, col);            
        }
    }
    return 0;
}

static u8
Pico_sget(i16 x, i16 y)
{
    if (x < 0 || x > 127) return 0;
    if (y < 0 || y > 127) return 0;
    return Pico_PackedGet(Pico.Gfx, y * 128 + x);
}

static int
Pico_sget(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2) 
        return 0;

    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    lua_pushnumber(L, INTtoFIX(Pico_sget(x, y)));
    return 1;
}

static void
Pico_sset(i16 x, i16 y, i16 col)
{
    if (x < 0 || x > 127) return;
    if (y < 0 || y > 127) return;
    if (col > 15 || col < 0) col = 0;
    Pico_PackedSet(Pico.Gfx, y * 128 + x, (u8)col);
}

static int
Pico_sset(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2) 
        return 0;

    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    i16 col = (args > 2) ? lua_tonumber(L, 3).i : Pico.Draw.Color;

    Pico_sset(x, y, col);

    return 0;
}

static void
Pico_sspr(i16 sx, i16 sy, i16 sw, i16 sh, 
          i16 dx, i16 dy, i16 dw, i16 dh, 
          b8 flip_x = false, i32 flip_y = false)
{
    i16 dx0 = dx;
    i16 dy0 = dy;
    i16 dx1 = dx + dw;
    i16 dy1 = dy + dh;

    r32 sx0, sy0, sx1, sy1;

    if (flip_x)
    {
        sx1 = (r32)(sx - 1);
        sx0 = (r32)(sx + sw - 1);
    }
    else
    {
        sx0 = sx;
        sx1 = (r32)(sx + sw);
    }

    if (flip_y)
    {
        sy1 = (r32)(sy - 1);
        sy0 = (r32)(sy + sh - 1);
    }
    else
    {
        sy0 = sy;
        sy1 = (r32)(sy + sh);
    }

    r32 sxd = (sx1 - sx0) / dw;
    r32 syd = (sy1 - sy0) / dh;

    r32 syr = sy0;
    for (i16 py = dy0; py < dy1; ++py, syr += syd)
    {
        r32 sxr = sx0;
        for (i16 px = dx0; px < dx1; ++px, sxr += sxd)
        {
            u8 col = Pico_sget((i16)sxr, (i16)syr);
            b8 t = Pico.Draw.PaletteDraw[col] >> 4;
            if (!t) Pico_pset(px, py, col);
        }
    }
}

static int
Pico_sspr(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 6) 
        return 0;

    i16 sx = lua_tonumber(L, 1).i;
    i16 sy = lua_tonumber(L, 2).i;
    i16 sw = lua_tonumber(L, 3).i;
    i16 sh = lua_tonumber(L, 4).i;
    i16 dx = lua_tonumber(L, 5).i;
    i16 dy = lua_tonumber(L, 6).i;
    i16 dw = (args > 6) ? lua_tonumber(L, 7).i : sw;
    i16 dh = (args > 7) ? lua_tonumber(L, 8).i : sh;
    b8 flip_x = (args > 7) ? (b8)lua_toboolean(L, 9) : false;
    b8 flip_y = (args > 8) ? (b8)lua_toboolean(L, 10) : false;

    Pico_sspr(sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);

    return 0;
}

static void
Pico_spr(i16 n, i16 x, i16 y, i16 w = 1, i16 h = 1, b8 flip_x = false, b8 flip_y = false)
{
    if (n < 0 || n > 255) return;

    i16 dx = x;
    i16 dw = w * 8;
    i16 dy = y;
    i16 dh = h * 8;
    
    i16 sy = (n / 16) * 8;
    i16 sx = (n % 16) * 8;
    i16 sw = 8;
    i16 sh = 8;

    Pico_sspr(sx, sy, sw, sh, dx, dy, dw, dh, flip_x, flip_y);
}

static int
Pico_spr(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 3) 
        return 0;

    i16 n = lua_tonumber(L, 1).i;
    i16 x = lua_tonumber(L, 2).i;
    i16 y = lua_tonumber(L, 3).i;
    i16 w = (args > 3) ? lua_tonumber(L, 4).i : 1;
    i16 h = (args > 4) ? lua_tonumber(L, 5).i : 1;
    b8 flip_x = (args > 5) ? (b8)lua_toboolean(L, 6) : false;
    b8 flip_y = (args > 6) ? (b8)lua_toboolean(L, 7) : false;
    
    Pico_spr(n, x, y, w, h, flip_x, flip_y);
    
    return 0;
}

static void
Pico_line(i16 x0, i16 y0, i16 x1, i16 y1, i16 col = Pico.Draw.Color)
{
    i16 absdx = (x1 > x0) ? x1 - x0 : x0 - x1;
    i16 absdy = (y1 > y0) ? y1 - y0 : y0 - y1;
    i16 steep = absdx < absdy;
    if (steep)
    {
        i16 t = x0;
        x0 = y0;
        y0 = t;
        t = x1;
        x1 = y1;
        y1 = t;
    }

    if (x0 > x1)
    {
        i16 t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }

    i16 dx = x1 - x0;
    i16 dx2 = 2 * dx;
    i16 dy = y1 - y0;
    i16 dyi = (dy < 0) ? -1 : 1;
    i16 dy2 = (dy < 0) ? -2 * dy : 2 * dy;
    i16 e = 0;

    if (steep)
    {
        i16 x = y0;
        for (i16 y = x0; y <= x1; ++y)
        {
            Pico_pset(x, y, col);
            e += dy2;
            if (e > dx)
            {
               x += dyi;
               e -= dx2;
            }
        }
    }
    else
    {
        i16 y = y0;
        for (i16 x = x0; x <= x1; ++x)
        {
            Pico_pset(x, y, col);
            e += dy2;
            if (e > dx)
            {
               y += dyi;
               e -= dx2;
            }
        }
    }
}

static int
Pico_line(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 4) 
        return 0;

    i16 x0 = lua_tonumber(L, 1).i;
    i16 y0 = lua_tonumber(L, 2).i;
    i16 x1 = lua_tonumber(L, 3).i;
    i16 y1 = lua_tonumber(L, 4).i;
    i16 col = (args > 4) ? lua_tonumber(L, 5).i : Pico.Draw.Color;

    Pico_line(x0, y0, x1, y1, col);

    return 0;
}

static void
Pico_circ(i16 x0, i16 y0, i16 r, i16 col = Pico.Draw.Color)
{
    i16 x = 0;
    i16 y = r;
    i16 dx = 0;
    i16 dy = -2 * r;
    i16 e = 1 - r;

    Pico_pset(x0, y0 + r, col);
    Pico_pset(x0, y0 - r, col);
    Pico_pset(x0 + r, y0, col);
    Pico_pset(x0 - r, y0, col);

    while (x < y)
    {
        if (e >= 0)
        {
            y--;
            dy += 2;
            e += dy;
        }
        x++;
        dx += 2;
        e += dx + 1;

        Pico_pset(x0 + x, y0 + y, col);
        Pico_pset(x0 + y, y0 + x, col);
        Pico_pset(x0 - x, y0 + y, col);
        Pico_pset(x0 - y, y0 + x, col);
        Pico_pset(x0 - x, y0 - y, col);
        Pico_pset(x0 - y, y0 - x, col);
        Pico_pset(x0 + x, y0 - y, col);
        Pico_pset(x0 + y, y0 - x, col);
    }
}

static int
Pico_circ(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 3) 
        return 0;

    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    i16 r = lua_tonumber(L, 3).i;
    i16 col = (args > 3) ? lua_tonumber(L, 4).i : Pico.Draw.Color;
    
    Pico_circ(x, y, r, col);

    return 0;
}

static void
Pico_circfill(i16 x0, i16 y0, i16 r, i16 col = Pico.Draw.Color)
{
    i16 x = 0;
    i16 y = r;
    i16 dx = 0;
    i16 dy = -2 * r;
    i16 e = 1 - r;

    for (i16 i = x; i <= y; ++i)
    {
        Pico_pset(x0, y0 + i, col);
        Pico_pset(x0, y0 - i, col);
        Pico_pset(x0 + i, y0, col);
        Pico_pset(x0 - i, y0, col);
    }

    while (x < y)
    {
        if (e >= 0)
        {
            y--;
            dy += 2;
            e += dy;
        }
        x++;
        dx += 2;
        e += dx + 1;

        for (i16 i = x; i <= y; ++i)
        {
            Pico_pset(x0 + x, y0 + i, col);
            Pico_pset(x0 + i, y0 + x, col);
            Pico_pset(x0 - x, y0 + i, col);
            Pico_pset(x0 - i, y0 + x, col);
            Pico_pset(x0 - x, y0 - i, col);
            Pico_pset(x0 - i, y0 - x, col);
            Pico_pset(x0 + x, y0 - i, col);
            Pico_pset(x0 + i, y0 - x, col);
        }
    }
}

static int
Pico_circfill(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 3) 
        return 0;

    i16 x = lua_tonumber(L, 1).i;
    i16 y = lua_tonumber(L, 2).i;
    i16 r = lua_tonumber(L, 3).i;
    i16 col = (args > 3) ? lua_tonumber(L, 4).i : Pico.Draw.Color;
    
    Pico_circfill(x, y, r, col);

    return 0;
}


static void
Pico_rect(i16 x0, i16 y0, i16 x1, i16 y1, i16 col = Pico.Draw.Color)
{
    if (x1 < x0)
    {
        i16 t = x0;
        x0 = x1;
        x1 = t;
    }

    for (i16 x = x0; x <= x1; ++x)
    {
        Pico_pset(x, y0, col);
        Pico_pset(x, y1, col);
    }

    if (y1 < y0)
    {
        i16 t = y0;
        y0 = y1;
        y1 = t;
    }

    for (i16 y = y0 + 1; y < y1; ++y)
    {
        Pico_pset(x0, y, col);
        Pico_pset(x1, y, col);
    }
}

static int
Pico_rect(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 4) 
        return 0;

    i16 x0 = lua_tonumber(L, 1).i;
    i16 y0 = lua_tonumber(L, 2).i;
    i16 x1 = lua_tonumber(L, 3).i;
    i16 y1 = lua_tonumber(L, 4).i;
    i16 col = (args > 4) ? lua_tonumber(L, 5).i : Pico.Draw.Color;

    Pico_rect(x0, y0, x1, y1, col);
    
    return 0;
}

static void
Pico_rectfill(i16 x0, i16 y0, i16 x1, i16 y1, i16 col = Pico.Draw.Color)
{
    for (i16 py = y0; py <= y1; ++py)
        for (i16 px = x0; px <= x1; ++px)
            Pico_pset(px, py, col);
}

static int
Pico_rectfill(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 4) 
        return 0;

    i16 x0 = lua_tonumber(L, 1).i;
    i16 y0 = lua_tonumber(L, 2).i;
    i16 x1 = lua_tonumber(L, 3).i;
    i16 y1 = lua_tonumber(L, 4).i;
    i16 col = (args > 4) ? lua_tonumber(L, 5).i : Pico.Draw.Color;

    Pico_rectfill(x0, y0, x1, y1, col);
    
    return 0;
}

static void
Pico_cls(i16 col = Pico.Draw.Color)
{
    Pico.Draw.CursorX = 0;
    Pico.Draw.CursorY = 0;

    if (col > 15 || col < 0) col = 0;
    u8 Color = ((u8)col << 4) | (u8)col;
    u8 *Pixel = Pico.Screen;
    for (i32 P = 0; P < 128 * 64; ++P)
        *Pixel++ = Color;
}

static int
Pico_cls(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
        Pico_cls();
    else
        Pico_cls(lua_tonumber(L, 1).i);

    return 0;
}

static void
Pico_camera(i16 x = 0, i16 y = 0)
{
    Pico.Draw.CameraX = x;
    Pico.Draw.CameraY = y;
}

static int
Pico_camera(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2)
        Pico_camera();
    else
    {
        i16 x = lua_tonumber(L, 1).i;
        i16 y = lua_tonumber(L, 2).i;
        Pico_camera(x, y);
    }
    return 0;
}

static void
Pico_color(i16 col = 0)
{
    if (col > 15 || col < 0) col = 0;
    Pico.Draw.Color = (u8)col;
}

static int
Pico_color(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
        Pico_color();
    else
        Pico_color(lua_tonumber(L, 1).i);
    return 0;
}

static void
Pico_clip(i16 x = 0, i16 y = 0, i16 w = 128, i16 h = 128)
{
    i16 x1 = x + w;
    i16 y1 = y + w;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > 127) x1 = 127;
    if (y1 > 127) y1 = 127;

    Pico.Draw.ClipX0 = (u8)x;
    Pico.Draw.ClipY0 = (u8)y;
    Pico.Draw.ClipX1 = (u8)x1;
    Pico.Draw.ClipY1 = (u8)y1;
}

static int
Pico_clip(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 4)
        Pico_clip();
    else
    {
        i16 x = lua_tonumber(L, 1).i;
        i16 y = lua_tonumber(L, 2).i;
        i16 w = lua_tonumber(L, 3).i;
        i16 h = lua_tonumber(L, 4).i;
        Pico_clip(x, y, w, h);
    }
    return 0;
}

static void
Pico_cursor(i16 x = 0, i16 y = 0)
{
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x > 127) x = 127;
    if (y > 127) y = 127;

    Pico.Draw.CursorX = (u8)x;
    Pico.Draw.CursorY = (u8)y;
}

static int
Pico_cursor(lua_State *L)
{
    i16 x = 0;
    i16 y = 0;
    i32 args = lua_gettop(L);
    switch (args)
    {
        case 2: y = lua_tonumber(L, 2).i;
        case 1: x = lua_tonumber(L, 1).i;
        case 0: Pico_cursor(x, y);
    }
    return 0;
}

static void
Pico_palt()
{
    for (i16 i = 0; i < 15; ++i)
    {
        Pico.Draw.PaletteDraw[i] &= 15;
    }
    Pico.Draw.PaletteDraw[0] |= 1 << 4;
}

static void
Pico_palt(i16 col, b8 t)
{
    if (col > 15 || col < 0) col = 0;
    Pico.Draw.PaletteDraw[col] &= 15;
    if (t)
        Pico.Draw.PaletteDraw[col] |= 1 << 4;
}

static int
Pico_palt(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        Pico_palt();
    }
    else if (args == 1)
    {
        // what happens when we only get a color?
    }
    else if (args > 1)
    {
        i16 col = lua_tonumber(L, 1).i;
        b8 t = (b8)lua_toboolean(L, 2);
        Pico_palt(col, t);
    }   

    return 0;
}

static void
Pico_pal()
{
    for (u8 i = 0; i < 16; ++i)
    {
        Pico.Draw.PaletteDraw[i] = i;
        Pico.Draw.PaletteScreen[i] = i;
    }
    Pico.Draw.PaletteDraw[0] |= 1 << 4;
}

static void
Pico_pal(i16 c0, i16 c1, i16 p = 0)
{
    if (c0 > 15 || c0 < 0) return;
    if (c1 > 15 || c1 < 0) return;
    if (p == 1)
    {
        Pico.Draw.PaletteScreen[c0] = (u8)c1;
    }
    else
    {
        Pico.Draw.PaletteDraw[c0] &= 15 << 4;
        Pico.Draw.PaletteDraw[c0] |= c1 & 15;
    }
}

static int
Pico_pal(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        Pico_pal();
    }
    else if (args == 1)
    {
        // only got one color
    }
    else if (args > 1)
    {
        i16 c0 = lua_tonumber(L, 1).i;
        i16 c1 = lua_tonumber(L, 2).i;
        i16 p = (args > 2) ? lua_tonumber(L, 3).i : 0;
        Pico_pal(c0, c1, p);
    }
    return 0;
}

static u8
Pico_fget(i16 n)
{
    if (n < 0 || n > 255) return 0;
    return Pico.GfxProps[n];
}

static b8
Pico_fget(i16 n, i16 f)
{
    if (n < 0 || n > 255) return 0;
    if (f < 0 || f > 7) return 0;
    return !!(Pico.GfxProps[n] & (1 << f));
}

static int
Pico_fget(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1)
    {
        return 0;
    }
    if (args == 1)
    {
        i16 n = lua_tonumber(L, 1).i;
        lua_pushnumber(L, INTtoFIX(Pico_fget(n)));
        return 1;
    }

    i16 n = lua_tonumber(L, 1).i;
    i16 f = lua_tonumber(L, 2).i;
    lua_pushboolean(L, Pico_fget(n, f));
    return 1;
}

static void
Pico_fset(i16 n, i16 f)
{
    if (n < 0 || n > 255) return;
    Pico.GfxProps[n] = (u8)f;
}

static void
Pico_fset(i16 n, i16 f, b8 v)
{
    if (n < 0 || n > 255) return;
    if (f < 0 || f > 7) return;
    Pico.GfxProps[n] &= (1 << f);
    Pico.GfxProps[n] |= ((!!v) << f);
}

static int
Pico_fset(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 2)
    {
        return 0;
    }
    if (args == 2)
    {
        i16 n = lua_tonumber(L, 1).i;
        i16 f = lua_tonumber(L, 2).i;
        Pico_fset(n, f);
        return 0;
    }

    i16 n = lua_tonumber(L, 1).i;
    i16 f = lua_tonumber(L, 2).i;
    b8 v = (b8)lua_toboolean(L, 3);
    Pico_fset(n, f, v);
    return 0;
}

#include "font.cpp"

static i16
Pico_print(const u8 c, i16 x, i16 y, u8 col)
{
    if (c < 32 || c > 153) return 0;
    u64 Bits = Font_chars[c - 32];
    if (Bits & 0x0000000001) Pico_pset(x    , y    , col);
    if (Bits & 0x0000000002) Pico_pset(x + 1, y    , col);
    if (Bits & 0x0000000004) Pico_pset(x + 2, y    , col);
    if (Bits & 0x0000000100) Pico_pset(x    , y + 1, col);
    if (Bits & 0x0000000200) Pico_pset(x + 1, y + 1, col);
    if (Bits & 0x0000000400) Pico_pset(x + 2, y + 1, col);
    if (Bits & 0x0000010000) Pico_pset(x    , y + 2, col);
    if (Bits & 0x0000020000) Pico_pset(x + 1, y + 2, col);
    if (Bits & 0x0000040000) Pico_pset(x + 2, y + 2, col);
    if (Bits & 0x0001000000) Pico_pset(x    , y + 3, col);
    if (Bits & 0x0002000000) Pico_pset(x + 1, y + 3, col);
    if (Bits & 0x0004000000) Pico_pset(x + 2, y + 3, col);
    if (Bits & 0x0100000000) Pico_pset(x    , y + 4, col);
    if (Bits & 0x0200000000) Pico_pset(x + 1, y + 4, col);
    if (Bits & 0x0400000000) Pico_pset(x + 2, y + 4, col);
    if (c < 128) return 4;
    if (Bits & 0x0000000008) Pico_pset(x + 3, y    , col);
    if (Bits & 0x0000000010) Pico_pset(x + 4, y    , col);
    if (Bits & 0x0000000020) Pico_pset(x + 5, y    , col);
    if (Bits & 0x0000000040) Pico_pset(x + 6, y    , col);
    if (Bits & 0x0000000800) Pico_pset(x + 3, y + 1, col);
    if (Bits & 0x0000001000) Pico_pset(x + 4, y + 1, col);
    if (Bits & 0x0000002000) Pico_pset(x + 5, y + 1, col);
    if (Bits & 0x0000004000) Pico_pset(x + 6, y + 1, col);
    if (Bits & 0x0000080000) Pico_pset(x + 3, y + 2, col);
    if (Bits & 0x0000100000) Pico_pset(x + 4, y + 2, col);
    if (Bits & 0x0000200000) Pico_pset(x + 5, y + 2, col);
    if (Bits & 0x0000400000) Pico_pset(x + 6, y + 2, col);
    if (Bits & 0x0008000000) Pico_pset(x + 3, y + 3, col);
    if (Bits & 0x0010000000) Pico_pset(x + 4, y + 3, col);
    if (Bits & 0x0020000000) Pico_pset(x + 5, y + 3, col);
    if (Bits & 0x0040000000) Pico_pset(x + 6, y + 3, col);
    if (Bits & 0x0800000000) Pico_pset(x + 3, y + 4, col);
    if (Bits & 0x1000000000) Pico_pset(x + 4, y + 4, col);
    if (Bits & 0x2000000000) Pico_pset(x + 5, y + 4, col);
    if (Bits & 0x4000000000) Pico_pset(x + 6, y + 4, col);
    return 8;
}

static void
Pico_print(const char* str, i16 x, i16 y, i16 col)
{
    if (col > 15 || col < 0) col = 0;
    Pico_cursor(x, y + 8);
    // TODO: scroll up if y>120
    while(*str)
    {
        x += Pico_print((u8)*str++, x, y, (u8)col);
    }
}

static int
Pico_print(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1 || !lua_isstring(L, 1)) 
        return 0;

    if (args < 3)
    {
        const char *str = lua_tostring(L, 1);
        Pico_print(str, Pico.Draw.CursorX, Pico.Draw.CursorY, Pico.Draw.Color);
    }
    else
    {
        const char *str = lua_tostring(L, 1);
        i16 x = lua_tonumber(L, 2).i;
        i16 y = lua_tonumber(L, 3).i;
        i16 col = (args > 3) ? lua_tonumber(L, 4).i : Pico.Draw.Color;
        Pico_print(str, x, y, col);
        Pico_cursor(x, y);
    }

    return 0;
}

static int
Pico_flip(lua_State *L)
{
    Win32_Flip(true);
    return 0;
}