#include <windows.h>

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
    HANDLE Cart = CreateFileW(L"cart.p8", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    HANDLE Sfx = CreateFileW(L"sfx.sfx", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (Exe == INVALID_HANDLE_VALUE)
        return 1;
    if (Cart == INVALID_HANDLE_VALUE)
        return 1;
    if (Sfx == INVALID_HANDLE_VALUE)
        return 1;

    BYTE Buffer[4096];
    DWORD BytesRead;
    while (ReadFile(Cart, Buffer, sizeof(Buffer), &BytesRead, 0) && BytesRead > 0)
    {
        FileAppend(Exe, Buffer, BytesRead);
    }

    BYTE Zero = 0;
    FileAppend(Exe, &Zero, 1);

    while (ReadFile(Sfx, Buffer, sizeof(Buffer), &BytesRead, 0) && BytesRead > 0)
    {
        FileAppend(Exe, Buffer, BytesRead);
    }

    return 0;
}