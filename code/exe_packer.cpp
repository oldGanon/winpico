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
#include <windows.h>
#include <lua/lua.hpp>
#include "pico.hpp"
#include "parse.cpp"

static void
FileAppend(HANDLE File, void *Buffer, size_t Bytes)
{
    DWORD BytesWritten;
    DWORD FilePos = SetFilePointer(File, 0, 0, FILE_END);
    LockFile(File, FilePos, 0, Bytes, 0);
    WriteFile(File, Buffer, Bytes, &BytesWritten, 0);
    UnlockFile(File, FilePos, 0, Bytes, 0);
}

int main(int argc, char *args)
{
    if (!CopyFileW(L"win32.exe", L"lemonhunter.exe", 0))
        return 1;

    HANDLE Exe = CreateFileW(L"lemonhunter.exe", FILE_APPEND_DATA, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (Exe == INVALID_HANDLE_VALUE)
        return 1;

    HANDLE Cart = CreateFileW(L"cart.p8", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (Cart == INVALID_HANDLE_VALUE)
        return 1;

    HANDLE Sfx = CreateFileW(L"sfx.sfx", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (Sfx == INVALID_HANDLE_VALUE)
        return 1;

    BYTE Buffer[4096];
#if 1
    char* P8;
    LARGE_INTEGER CartSize;
    GetFileSizeEx(Cart, &CartSize);
    u32 CartSize32 = (u32)CartSize.QuadPart;
    P8 = (char *)malloc(CartSize32+1);
    DWORD BytesRead;
    if(!ReadFile(Cart, P8, CartSize32, &BytesRead, 0) || (CartSize32 != BytesRead))
        return 1;
    P8[CartSize32] = 0;

    pico_cart *PicoCart = Parse_P8(P8);

    FileAppend(Exe, PicoCart, Pico_CartSize(PicoCart));
#else
    DWORD BytesRead;
    while (ReadFile(Cart, Buffer, sizeof(Buffer), &BytesRead, 0) && BytesRead > 0)
        FileAppend(Exe, Buffer, BytesRead);

    BYTE Zero = 0;
    FileAppend(Exe, &Zero, 1);
#endif

    while (ReadFile(Sfx, Buffer, sizeof(Buffer), &BytesRead, 0) && BytesRead > 0)
        FileAppend(Exe, Buffer, BytesRead);
    
    return 0;
}