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
Parse_Sfx(char* Data)
{
    i16 Sfx = 0;
    char *Line = Parse_SfxPopLine(&Data);
    while (Line && Sfx < 64)
    {
        PicoCart.Sfx[Sfx].EditorMode = Parse_HexByte(Line);
        PicoCart.Sfx[Sfx].Speed = Parse_HexByte(Line + 2);
        PicoCart.Sfx[Sfx].Start = Parse_HexByte(Line + 4);
        PicoCart.Sfx[Sfx].End = Parse_HexByte(Line + 6);

        for (i16 n = 0; n < 32; n++)
        {
            u8 P = Parse_HexByte(Line + 8 + (n * 5));
            u8 W = Parse_Hex(*(Line + 8 + (n * 5) + 2));
            u8 V = Parse_Hex(*(Line + 8 + (n * 5) + 3));
            u8 E = Parse_Hex(*(Line + 8 + (n * 5) + 4));
            
            PicoCart.Sfx[Sfx].Notes[n] = (E << 12) | (V << 9) | (W << 6) | P;
        }

        Line = Parse_SfxPopLine(&Data);
        ++Sfx;
    }
}
