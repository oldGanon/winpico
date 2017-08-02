const r32 NotePitches[65] =
{
/* NOTE         HZ          SAMPLES
/* C -0 */      65.4064f,   // 733.87
/* C#-0 */      69.2957f,   //
/* D -0 */      73.4162f,   //
/* D#-0 */      77.7817f,   //
/* E -0 */      82.4069f,   //
/* F -0 */      87.3071f,   //
/* F#-0 */      92.4986f,   //
/* G -0 */      97.9989f,   //
/* G#-0 */      103.826f,   //
/* A -0 */      110.000f,   // 436.36
/* A#-0 */      116.541f,   //
/* B -0 */      123.471f,   //

/* C -1 */      130.813f,   //
/* C#-1 */      138.591f,   //
/* D -1 */      146.832f,   //
/* D#-1 */      155.563f,   //
/* E -1 */      164.814f,   //
/* F -1 */      174.614f,   //
/* F#-1 */      184.997f,   //
/* G -1 */      195.998f,   //
/* G#-1 */      207.652f,   //
/* A -1 */      220.000f,   // 218.18
/* A#-1 */      233.082f,   //
/* B -1 */      246.942f,   //

/* C -2 */      261.626f,   //
/* C#-2 */      277.183f,   //
/* D -2 */      293.665f,   //
/* D#-2 */      311.127f,   //
/* E -2 */      329.628f,   //
/* F -2 */      349.228f,   //
/* F#-2 */      369.994f,   //
/* G -2 */      391.995f,   //
/* G#-2 */      415.305f,   //
/* A -2 */      440.000f,   // 109.09
/* A#-2 */      466.164f,   //
/* B -2 */      493.883f,   //

/* C -3 */      523.251f,   //
/* C#-3 */      554.365f,   //
/* D -3 */      587.330f,   //
/* D#-3 */      622.254f,   //
/* E -3 */      659.255f,   //
/* F -3 */      698.456f,   //
/* F#-3 */      739.989f,   //
/* G -3 */      783.991f,   //
/* G#-3 */      830.609f,   //
/* A -3 */      880.000f,   // 54.54
/* A#-3 */      932.328f,   //
/* B -3 */      987.767f,   //

/* C -4 */      1046.50f,   //
/* C#-4 */      1108.73f,   //
/* D -4 */      1174.66f,   //
/* D#-4 */      1244.51f,   //
/* E -4 */      1318.51f,   //
/* F -4 */      1396.91f,   //
/* F#-4 */      1479.98f,   //
/* G -4 */      1567.98f,   //
/* G#-4 */      1661.22f,   //
/* A -4 */      1760.00f,   // 27.27
/* A#-4 */      1864.66f,   //
/* B -4 */      1975.53f,   // 24.29

/* C -5 */      2093.00f,   // 22.93
/* C#-5 */      2217.46f,   // 21.64
/* D -5 */      2349.32f,   // 20.43
/* D#-5 */      2489.02f,   // 19.28
/* E -5 */      2793.83f,   //
};

enum audio_waveform
{
    AUDIO_WAVEFORM_SINE         = 0,
    AUDIO_WAVEFORM_TRIANGLE     = 1,
    AUDIO_WAVEFORM_SAWTOOTH     = 2,
    AUDIO_WAVEFORM_LONG_SQUARE  = 3,
    AUDIO_WAVEFORM_SHORT_SQUARE = 4,
    AUDIO_WAVEFORM_RINGING      = 5,
    AUDIO_WAVEFORM_NOISE        = 6,
    AUDIO_WAVEFORM_RINGING_SINE = 7,
};

enum audio_effect
{
    AUDIO_EFFECT_NONE           = 0,
    AUDIO_EFFECT_SLIDE          = 1,
    AUDIO_EFFECT_VIBRATO        = 2,
    AUDIO_EFFECT_DROP           = 3,
    AUDIO_EFFECT_FADE_IN        = 4,
    AUDIO_EFFECT_FADE_OUT       = 5,
    AUDIO_EFFECT_ARPEGGIO_FAST  = 6,
    AUDIO_EFFECT_ARPEGGIO_SLOW  = 7,
};

//
// AUDIO
//

struct playing_sfx
{
    u32 Phase;
    u32 Samples;
    i8 SfxIndex;
};

static playing_sfx Channels[4] = {{0,0,-1},{0,0,-1},{0,0,-1},{0,0,-1}};

static int
Pico_sfx(lua_State *L)
{
    i32 args = lua_gettop(L);
    if (args < 1) 
        return 0;
    
    i16 n = lua_tonumber(L, 1).i;
    if (n < 0 || n > 63) return 0;

    i16 channel = -1;
    i16 offset = 0;
    if (args > 1)
        channel =  lua_tonumber(L, 2).i;
    if (args > 2) 
        offset = lua_tonumber(L, 3).i;

    if (channel == -1)
    {
        for (i16 c = 0; c < 4; ++c)
        {
            if (Channels[c].SfxIndex == -1)
            {
                channel = c;
                break;
            }
        }
    }

    if (channel < 0 || channel > 3) 
        return 0;

    Channels[channel].SfxIndex = (u8)n;
    Channels[channel].Samples = 0;
    Channels[channel].Phase = 0;

    return 0;
}

struct note_info
{
    i16 Pitch;
    i16 Volume;
    i16 Waveform;
    i16 Effect;
};

#define SFX_GET_EFFECT(n)   (((n)&0x7000)>>12)
#define SFX_GET_VOLUME(n)   (((n)&0xE00)>>9)
#define SFX_GET_WAVEFORM(n) (((n)&0x1C0)>>6)
#define SFX_GET_PITCH(n)    (((n)&0x3F))

inline note_info
Pico_GetNoteInfo(i16 Note)
{
    note_info Result;
    Result.Pitch = SFX_GET_PITCH(Note);
    Result.Volume = SFX_GET_VOLUME(Note);
    Result.Waveform = SFX_GET_WAVEFORM(Note);
    Result.Effect = SFX_GET_EFFECT(Note);
    return Result;
}

inline i16
Pico_SineNote(i64 Position)
{
    i16 Value = (Position % 65532) - 32766;
    if (Value < 0) 
        Value = -Value;
    return (Value - 16383) * 2;
}

inline i16
Pico_LongSquareNote(i64 Position)
{
    i16 Value = (Position % 65532 - 32766) < 0 ? -32766 : 32766;
    return Value;
}

inline i16
Pico_ShortSquareNote(i64 Position)
{
    i16 Value = (Position % 65532 - 21844) < 0 ? -32766 : 32766;
    return Value;
}

inline i16
Pico_SawtoothNote(i64 Position)
{
    i16 Value = (Position % 65532) - 32766;
    return Value;
}

inline i16
Pico_TiltedSawNote(i64 Position)
{
    Position %= 65532;
    i16 Value = (i16)(Position * 8 / 7);
    if (Position > 57340)
        Value -= (Value - 57341) * 8;
    Value -= 32766;
    return Value;
}

inline i16
Pico_OrganNote(i64 Position)
{
    i16 Value = (Position % 65532) - 32766;
    if (Value < 0) 
        Value = -Value;
    if ((Position / 65532) % 2)
    {
        Value += 32766;
        Value = Value * 4 / 6;
        Value -= 32766;
    }
    return (Value - 16383) * 2;
}

inline i16
Pico_PhaserNote(i64 Position)
{
    i16 Value1 = (Position % 65532) - 32766;
    if (Value1 < 0) 
        Value1 = -Value1;
    Value1 -= 16383;

    i16 Value2 = (Position % 64986) - 32493;
    if (Value2 > 0) 
        Value2 = -Value2;
    Value2 /= 2;

    return Value1 + Value2;
}

#include "noise.cpp"

inline i16
Pico_NoiseNote(i64 Position)
{
    i16 Value = 0;
    Value += (i16)(Noise(Position / 32766.0f) * 16383);
    Value += (i16)(Noise(Position / 16383.0f) * 8192);
    Value += (i16)(Noise(Position / 8192.0f) * 4096);
    Value += (i16)(Noise(Position / 4096.0f) * 2048);
    return Value;
}

static i16
Audio_ValueNote(note_info NoteInfo, u32 SamplesPerSecond, u32 Samples, u32 *Phase)
{

    if (NoteInfo.Effect == AUDIO_EFFECT_VIBRATO)
    {
        *Phase += (u64)(NotePitches[NoteInfo.Pitch] * 65532) / SamplesPerSecond;
        r32 Var = Pico_SineNote((Samples * 65532 * 4) / SamplesPerSecond) / 32766.0f;
        if (Var < 0.0f)
        {
            r32 Difference = NotePitches[NoteInfo.Pitch] - NotePitches[NoteInfo.Pitch-1];
            *Phase -= (u64)(((Difference * 65532) / SamplesPerSecond) * Var);
        }
        else
        {
            r32 Difference = NotePitches[NoteInfo.Pitch+1] - NotePitches[NoteInfo.Pitch];
            *Phase += (u64)(((Difference * 65532) / SamplesPerSecond) * Var);
        }
    }
    else if (NoteInfo.Effect == AUDIO_EFFECT_DROP)
    {
        *Phase += (u64)(NotePitches[NoteInfo.Pitch] * 65532) / SamplesPerSecond;
    }
    else
    {
        *Phase += (u64)(NotePitches[NoteInfo.Pitch] * 65532) / SamplesPerSecond;
    }

    i16 Value = 0;

    switch (NoteInfo.Waveform)
    {
        case AUDIO_WAVEFORM_SINE:           Value = Pico_SineNote(*Phase); break;
        case AUDIO_WAVEFORM_TRIANGLE:       Value = Pico_TiltedSawNote(*Phase) / 2; break;
        case AUDIO_WAVEFORM_SAWTOOTH:       Value = Pico_SawtoothNote(*Phase) / 3; break;
        case AUDIO_WAVEFORM_LONG_SQUARE:    Value = Pico_ShortSquareNote(*Phase) / 3; break;
        case AUDIO_WAVEFORM_SHORT_SQUARE:   Value = Pico_ShortSquareNote(*Phase) / 3; break;
        case AUDIO_WAVEFORM_RINGING:        Value = Pico_OrganNote(*Phase); break;
        case AUDIO_WAVEFORM_NOISE:          Value = Pico_NoiseNote(*Phase); break;
        case AUDIO_WAVEFORM_RINGING_SINE:   Value = Pico_PhaserNote(*Phase); break;
    }

    return Value;
}

static void
Audio_OutputSamples(game_sound_output *SoundBuffer)
{
    for (i16 c = 0; c < 4; ++c)
    {
        if (Channels[c].SfxIndex == -1)
            continue;

        playing_sfx *Sound = &Channels[c];
        game_sound_output ChannelBuffer = *SoundBuffer;
        while (ChannelBuffer.SampleCount > 0 && Sound->SfxIndex != -1)
        {
            sound_effect *Sfx = &Pico.Sfx[Sound->SfxIndex];
            i32 SamplesPerSecond = ChannelBuffer.SamplesPerSecond;
            i32 SamplesPerNote = (Sfx->Speed * SamplesPerSecond) / 128;
            i32 SamplesIntoNote = Sound->Samples % SamplesPerNote;
            i32 SamplesLeftInNote = SamplesPerNote - SamplesIntoNote;
            u8 NoteIndex = (Sound->Samples / SamplesPerNote) % 32;
            note_info NoteInfo = Pico_GetNoteInfo(Sfx->Notes[NoteIndex]);

            do
            {
                r32 Value = (r32)Audio_ValueNote(NoteInfo, SamplesPerSecond, Sound->Samples++, &Sound->Phase);
                Value = Value * (NoteInfo.Volume / 32.0f);

                if (NoteInfo.Effect == AUDIO_EFFECT_FADE_IN)
                    Value *= SamplesIntoNote / (r32)SamplesPerNote;
                else if (NoteInfo.Effect == AUDIO_EFFECT_FADE_OUT)
                    Value *= SamplesLeftInNote / (r32)SamplesPerNote;

                if (SamplesIntoNote < 64)
                    Value *= SamplesIntoNote / 64.0f;
                else if (SamplesLeftInNote < 64)
                    Value *= SamplesLeftInNote / 64.0f;
                
                *ChannelBuffer.Samples++ += (i16)Value;
                *ChannelBuffer.Samples++ += (i16)Value;
             
                ++SamplesIntoNote;
                if (--SamplesLeftInNote <= 0)
                {
                    NoteIndex = (Sound->Samples / SamplesPerNote) % 32;
                    NoteInfo = Pico_GetNoteInfo(Sfx->Notes[NoteIndex]);
                    if (NoteInfo.Pitch == 0 ||
                        NoteIndex == Sfx->End)
                    {
                        Sound->Samples = 0;
                        Sound->Phase = 0;
                        Sound->SfxIndex = -1;
                        return;
                    }
                    break;
                }
            }
            while (--ChannelBuffer.SampleCount);
        }
    }

    for (i64 S = 1; S < SoundBuffer->SampleCount - 1; ++S)
    {
        i64 I = S*2;
        SoundBuffer->Samples[I  ] = SoundBuffer->Samples[I  ] / 2 + SoundBuffer->Samples[I-2] / 2;
        SoundBuffer->Samples[I+1] = SoundBuffer->Samples[I+1] / 2 + SoundBuffer->Samples[I-1] / 2;
    }
}
