static i8
Parse_Hex(const char C)
{
    if ('0' <= C && C <= '9')
    {
        return (C - '0');
    }
    else if ('a' <= C && C <= 'f')
    {
        return (C - 'a' + 10);
    }
    else return -1;
}

static u8
Parse_HexByte(const char *S)
{
    u8 Hi = Parse_Hex(S[0]);
    u8 Lo = Parse_Hex(S[1]);

    if (Lo < 0 || Hi < 0)
        return 0;

    return (Hi << 4 | Lo);
}

static char*
Parse_SfxPopLine(char** Data)
{
    char *Start = *Data;
    char *End = *Data;
    while (Parse_Hex(*End) != -1)
    {
        ++End;
    }

    mi LineLength = End - Start;
    if (LineLength == 168)
    {
        *Data = End + 1;
        return Start;
    }
    return 0;
}

static void
Parse_Sfx(char* Data, sound_effect *SfxMem)
{
    i16 Sfx = 0;
    char *Line = Parse_SfxPopLine(&Data);
    while (Line && Sfx < 64)
    {
        SfxMem[Sfx].EditorMode = Parse_HexByte(Line);
        SfxMem[Sfx].Speed = Parse_HexByte(Line + 2);
        SfxMem[Sfx].Start = Parse_HexByte(Line + 4);
        SfxMem[Sfx].End = Parse_HexByte(Line + 6);

        for (i16 n = 0; n < 32; n++)
        {
            u8 P = Parse_HexByte(Line + 8 + (n * 5));
            u8 W = Parse_Hex(*(Line + 8 + (n * 5) + 2));
            u8 V = Parse_Hex(*(Line + 8 + (n * 5) + 3));
            u8 E = Parse_Hex(*(Line + 8 + (n * 5) + 4));
            
            SfxMem[Sfx].Notes[n] = (E << 12) | (V << 9) | (W << 6) | P;
        }

        Line = Parse_SfxPopLine(&Data);
        ++Sfx;
    }
}

static void
Parse_StringData(char* Data, u8 *Memory)
{
    i32 X = 0;
    char *P = Data - 1;;
    while (*(++P))
    {
        u8 Value = 0;
        if ('0' <= *P && *P <= '9') 
            Value = *P - '0';
        else if ('a' <= *P && *P <= 'f')
            Value = *P - 'a' + 10;
        else continue;

        Pico_PackedSet(Memory, X++, Value);
    }
}

static int
Parse_Writer(lua_State *L, const void* Src, size_t Size, void* Data)
{
    pico_cart **Cart = (pico_cart **)Data;
    *Cart = (pico_cart *)realloc(*Cart, Pico_CartSize(*Cart) + Size);
    memcpy((*Cart)->Code + (*Cart)->CodeSize, Src, Size);
    (*Cart)->CodeSize += Size;
    return 0;
}

static pico_cart*
Parse_P8(char* P8)
{
    char *lua = 0;
    char *gfx = 0;
    char *gff = 0;
    char *map = 0;
    char *sfx = 0;
    char *label = 0;
    char *music = 0;

    char* P = P8;
    while (*P)
    {
        if (P[0] == '_' && P[1] == '_' && P[5] == '_' && P[6] == '_')
        {
            *(P) = 0;
            if (P[2] == 'l' && P[3] == 'u' && P[4] == 'a') lua = &P[7] + 1;
            if (P[2] == 'g' && P[3] == 'f' && P[4] == 'x') gfx = &P[7] + 1;
            if (P[2] == 'g' && P[3] == 'f' && P[4] == 'f') gff = &P[7] + 1;
            if (P[2] == 'm' && P[3] == 'a' && P[4] == 'p') map = &P[7] + 1;
            if (P[2] == 's' && P[3] == 'f' && P[4] == 'x') sfx = &P[7] + 1;
        }

        if (P[0] == '_' && P[1] == '_' && P[7] == '_' && P[8] == '_')
        {
            *(P) = 0;
            if (P[2] == 'l' && P[3] == 'a' && P[4] == 'b' && P[5] == 'e' && P[6] == 'l') label = &P[7] + 1;
            if (P[2] == 'm' && P[3] == 'u' && P[4] == 's' && P[5] == 'i' && P[6] == 'c') music = &P[7] + 1;
        }

        ++P;
    }

    pico_cart *Cart = (pico_cart *)malloc(sizeof(pico_cart) - 1);
    memset(Cart, 0x00, sizeof(pico_cart));

    // LUA
    lua_State *L = luaL_newstate();
    if (luaL_loadstring(L, lua))
    {
        const char *ErrStr = lua_tostring(L, -1);
        free(Cart);
        return 0;
    }
    else
    {
        if (lua_dump(L, Parse_Writer, &Cart))
        {
            free(Cart);
            return 0;
        }
    }
    lua_pop(L, 1);
    lua_close(L);

    // relying on correctly formated cart, if its not everything explodes
    Parse_StringData(gfx, Cart->Data.Gfx);
    Parse_StringData(map, Cart->Data.Map);
    Parse_StringData(gff, Cart->Data.GfxProps);
    Parse_Sfx(sfx, Cart->Data.Sfx);

    return Cart;
}