#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

typedef int8_t b8;

typedef size_t mi;

#if GAME_SLOW
#define Assert(Expression) if (!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#include <windows.h>

#pragma pack(push, 1)
struct WAVE_header
{
    u32 RIFFID;
    u32 Size;
    u32 WAVEID;
};

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
enum
{
    WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
    WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
    WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
    WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};
struct WAVE_chunk
{
    u32 ID;
    u32 Size;
};

struct WAVE_fmt
{
    u16 wFormatTag;
    u16 nChannels;
    u32 nSamplesPerSec;
    u32 nAvgBytesPerSec;
    u16 nBlockAlign;
    u16 wBitsPerSample;
    u16 cbSize;
    u16 wValidBitsPerSample;
    u32 dwChannelMask;
    u8 SubFormat[16];
};
#pragma pack(pop)


struct riff_iterator
{
    u8 *At;
    u8 *Stop;
};

inline riff_iterator
ParseChunkAt(void *At, void *Stop)
{
    riff_iterator Iter;

    Iter.At = (u8 *)At;
    Iter.Stop = (u8 *)Stop;

    return(Iter);
}

inline riff_iterator
NextChunk(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    u32 Size = (Chunk->Size + 1) & ~1;
    Iter.At += sizeof(WAVE_chunk) + Size;

    return(Iter);
}
             
inline b8
IsValid(riff_iterator Iter)
{    
    b8 Result = (Iter.At < Iter.Stop);
    
    return(Result);
}

inline void *
GetChunkData(riff_iterator Iter)
{
    void *Result = (Iter.At + sizeof(WAVE_chunk));

    return(Result);
}

inline u32
GetType(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    u32 Result = Chunk->ID;

    return(Result);
}

inline u32
GetChunkDataSize(riff_iterator Iter)
{
    WAVE_chunk *Chunk = (WAVE_chunk *)Iter.At;
    u32 Result = Chunk->Size;

    return(Result);
}

static void*
LoadEntireFile(const wchar_t* Filename)
{
    HANDLE FileHandle = CreateFileW(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle == INVALID_HANDLE_VALUE)
        return 0;

    void *Result = 0;
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize))
    {
        u32 FileSize32 = (u32)FileSize.QuadPart;
        Result = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        if(Result)
        {
            DWORD BytesRead;
            if(!ReadFile(FileHandle, Result, FileSize32, &BytesRead, 0) || (FileSize32 != BytesRead))
            {
                VirtualFree(Result, 0, MEM_RELEASE);
                Result = 0;
            }
        }
    }
        
    CloseHandle(FileHandle);
    return Result;
}

static void*
LoadSfxFile(wchar_t *Filename)
{
    void *ReadResult = LoadEntireFile(Filename);
    if(ReadResult)
    {        
        WAVE_header *Header = (WAVE_header *)ReadResult;
        Assert(Header->RIFFID == WAVE_ChunkID_RIFF);
        Assert(Header->WAVEID == WAVE_ChunkID_WAVE);

        size_t SampleSize = sizeof(i16);

        i16 *SampleData = 0;
        u32 SampleDataSize = 0;
        for (riff_iterator Iter = ParseChunkAt(Header + 1, (u8 *)(Header + 1) + Header->Size - 4);
            IsValid(Iter);
            Iter = NextChunk(Iter))
        {
            switch(GetType(Iter))
            {
                case WAVE_ChunkID_fmt:
                {
                    WAVE_fmt *fmt = (WAVE_fmt *)GetChunkData(Iter);
                    Assert(fmt->wFormatTag == 1); // NOTE: Only support PCM
                    Assert(fmt->nSamplesPerSec == 22050);
                    Assert(fmt->wBitsPerSample == 16);
                    Assert(fmt->nBlockAlign == SampleSize);
                    Assert(fmt->nChannels == 1);
                } break;

                case WAVE_ChunkID_data:
                {
                    SampleData = (i16 *)GetChunkData(Iter);
                    SampleDataSize = GetChunkDataSize(Iter);
                } break;
            }
        }

        u32 SampleCount = (u32)(SampleDataSize / SampleSize);
        i16 *LastSample = SampleData + SampleCount - 1;
        while (*LastSample-- == 0)
            --SampleCount;
        SampleDataSize = (u32)(SampleCount * SampleSize);
    
        u8 *SfxMemory = (u8 *)VirtualAlloc(0, sizeof(u32) + SampleDataSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        *(u32 *)SfxMemory = SampleCount;
        memcpy(SfxMemory + sizeof(u32), SampleData, SampleDataSize);

        return SfxMemory;
    }
    return 0;
}

int main(int argc, char *args)
{
	HANDLE SfxFile = CreateFileW(L"sfx.sfx", FILE_APPEND_DATA, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    Assert(SfxFile != INVALID_HANDLE_VALUE);

    SetCurrentDirectoryW(L"sfx");

    WIN32_FIND_DATAW FindData;
    HANDLE FindHandle = FindFirstFileW(L"*.wav", &FindData);
    while (FindHandle != INVALID_HANDLE_VALUE)
    {
        void *Data = LoadSfxFile(FindData.cFileName);
        if (!Data)
            return 1;

        u32 SampleCount = *(u32 *)Data;
        u32 DataSize = SampleCount * sizeof(i16) + sizeof(u32);
        DWORD FilePos = SetFilePointer(SfxFile, 0, 0, FILE_END);
        LockFile(SfxFile, FilePos, 0, DataSize, 0);
        WriteFile(SfxFile, Data, DataSize, 0, 0);
        UnlockFile(SfxFile, FilePos, 0, DataSize, 0);

        if (!FindNextFileW(FindHandle, &FindData))
            break;
    }
    FindClose(FindHandle);

    SetCurrentDirectoryW(L"..");
}
