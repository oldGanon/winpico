struct sfx
{
	u32 SampleCount;
	i16 Samples[1];
};

struct sfx_channel
{
	sfx *Sfx;
	u32 Samples;
};

static sfx *SfxTable[64] = {};
static sfx_channel Channels[4] = {};

static int
Pico_sfx(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1) 
        return 0;
    
    i16 n = lua_tonumber(L, 1).i;
    if (n < 0 || n > 63) return 0;

    if (args > 1)
    {
        i16 c =  lua_tonumber(L, 2).i;
        Channels[c].Sfx = SfxTable[n];
		Channels[c].Samples = 0;
        return 0;
    }
    else
    {
        for (i16 c = 0; c < 4; ++c)
        {
            if (!Channels[c].Sfx)
            {
			    Channels[c].Sfx = SfxTable[n];
			    Channels[c].Samples = 0;
                return 0;
            }
        }
    }

    i16 offset = 0;
    if (args > 2) 
        offset = lua_tonumber(L, 3).i;

    return 0;
}

static void
Audio_LoadSfx(u8 *Sfx)
{
	for (i32 i = 0; i < 64; ++i)
	{
		SfxTable[i] = (sfx *)Sfx;
		Sfx += SfxTable[i]->SampleCount * sizeof(i16) + sizeof(u32);
	}
}

static void
Audio_OutputSamples(game_sound_output *SoundBuffer)
{
	for (i16 c = 0; c < 4; ++c)
    {
        if (!Channels[c].Sfx)
            continue;

        sfx_channel *Channel = Channels + c;
        i16 *Samples = SoundBuffer->Samples;
        u32 SamplesLeft = Channel->Sfx->SampleCount - Channel->Samples;
        u32 SampesToWrite = MIN(SoundBuffer->SampleCount, SamplesLeft);
        while (SampesToWrite--)
        {
        	i16 Sample = Channel->Sfx->Samples[Channel->Samples++] / 4;
        	*Samples++ += Sample;
        	*Samples++ += Sample;

        	if (Channel->Samples == Channel->Sfx->SampleCount)
        		Channel->Sfx = 0;
        }
    }
}