struct draw_state
{
    // 64-bytes
    u8 PaletteDraw[16];
    u8 PaletteScreen[16];
    u8 ClipX0;
    u8 ClipY0;
    u8 ClipX1;
    u8 ClipY1;
    u8 UNUSED;
    u8 Color;
    u8 CursorX;
    u8 CursorY;
    i16 CameraX;
    i16 CameraY;
    u8 Padding[19];
};

enum button_bits
{
    BUTTON_BIT_LEFT   = 0,
    BUTTON_BIT_RIGHT  = 1,
    BUTTON_BIT_UP     = 2,
    BUTTON_BIT_DOWN   = 3,
    BUTTON_BIT_CIRCLE = 4,
    BUTTON_BIT_CROSS  = 5,
};

struct hardware_state
{
    // 64-bytes
    u8 Keyboards[2];
    u8 Controllers[2];
    u8 Padding[60];
};

struct sound_effect
{
    // 68-bytes
    u8 EditorMode;
    u8 Speed;
    u8 Start;
    u8 End;
    u16 Notes[32];
};

struct pico_cart_data
{
    u8 Gfx[4096];
    u8 Gfx2[4096];
    u8 Map[4096];
    u8 GfxProps[256];
    u8 Song[256];
    sound_effect Sfx[64];
};

struct pico_state
{
    u8 Gfx[4096];
    u8 Gfx2[4096];
    u8 Map[4096];
    u8 GfxProps[256];
    u8 Song[256];
    sound_effect Sfx[64];

    u8 UserData[6912];
    u8 PersistentCartData[256];
    draw_state Draw;
    hardware_state Hardware;
    u8 GpioPins[128];
    u8 Screen[8192];
};

struct pico_cart
{
    pico_cart_data Data;
    size_t CodeSize;
    u8 Code[1];
};

inline size_t
Pico_CartSize(pico_cart *Cart)
{
    return sizeof(pico_cart) + Cart->CodeSize - 1;
}

#define INTtoFIX(i) { (i)<<FIXED_SHIFT }

inline void
Pico_PackedSet(u8 *Data, u32 Offset, u8 Value)
{
    u32 RealOffset = Offset/2;
    if (Offset & 1)
    {
        Data[RealOffset] &= 15 << 4;
        Data[RealOffset] |= Value;
    }
    else
    {
        Data[RealOffset] &= 15;
        Data[RealOffset] |= Value << 4;
    }
}

inline u8
Pico_PackedGet(u8 *Data, u32 Offset)
{
    u32 RealOffset = Offset/2;
    if (Offset & 1)
        return Data[RealOffset] & 15;
    else
        return Data[RealOffset] >> 4;
}