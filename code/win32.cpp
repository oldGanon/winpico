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
typedef size_t memory_index;

#define Align16(Value) ((Value + 15) & ~15)

struct game_sound_output
{
    u32 SamplesPerSecond;
    u32 SampleCount;

    i16 *Samples;
};

#include <windows.h>
#include <xinput.h>
#include <dsound.h>

static void Win32_Flip(b8 CalledFromLUA);

#include <lua/lua.hpp>
#include "pico.c"

struct win32_sound_output
{
    u32 SamplesPerSecond;
    u32 Cursor;
    u32 BytesPerSample;
    DWORD SecondaryBufferSize;
};

#define FRAMES_PER_SECOND 60
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define SCREEN_BYTES_PER_PIXEL 4
#define SCREEN_PITCH (Align16(SCREEN_WIDTH * SCREEN_BYTES_PER_PIXEL))

struct bitmap
{
    i16 Width;
    i16 Height;
    i16 Pitch; //bytes per horizontal pixel line
    i16 BytesPerPixel;
    void *Pixels;
};

static bitmap RenderBuffer;

static b8 GlobalRunning;
static b8 GlobalFocus;
static i64 GlobalPerfCountFrequency;
static u64 GlobalLastCounter;
static win32_sound_output GlobalSoundOutput = {};
static const r32 GameUpdateHz = (r32)FRAMES_PER_SECOND;
static const r32 TargetSecondsPerFrame = 1.0f / (r32)GameUpdateHz;
static HWND GlobalWindow;
static BITMAPINFO GlobalBackbufferInfo;
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
static WINDOWPLACEMENT GlobalWindowPosition = {sizeof(GlobalWindowPosition)};

// NOTE: XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

inline void* 
Memory_Set(void *Ptr, u8 Value, memory_index Size)
{
    u8 *Byte = (u8 *)Ptr;
    while (Size--) *Byte++ = Value;
    return Ptr;
}

inline void* 
Memory_Copy(void *Dest, void *Source, memory_index Num)
{
    u8 *DestByte = (u8 *)Dest;
    u8 *SourceByte = (u8 *)Source;
    while (Num--) *DestByte++ = *SourceByte++;
    return Dest;
}

static void
Win32_FreeMemory(void *Memory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

static void*
Win32_LoadEntireFile(const char* Filename)
{
    HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle == INVALID_HANDLE_VALUE)
        return 0;

    void *Result = 0;
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize))
    {
        u32 FileSize32 = (u32)FileSize.QuadPart;
        Result = VirtualAlloc(0, FileSize32+1, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
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

static lua_State*
Win32_LoadCart(char *Filename)
{
    lua_State *L = luaL_newstate();
    char *Cart = (char *)Win32_LoadEntireFile(Filename);
    Pico_Init(L, Cart);
    Win32_FreeMemory(Cart);
    return L;
}

static void
Win32_UnloadCart(lua_State *L)
{
    lua_close(L);
}

static void
Win32_ClearBuffer()
{
    VOID *Region1; DWORD Region1Size;
    VOID *Region2; DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, 0,
                                              &Region1, &Region1Size,
                                              &Region2, &Region2Size,
                                              DSBLOCK_ENTIREBUFFER)))
    {
        Memory_Set(Region1, 0, Region1Size);
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

static void
Win32_InitDSound(HWND Window, i32 SamplesPerSecond, i32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (!DSoundLibrary)
    {
        return;
    }

    direct_sound_create *DirectSoundCreate = (direct_sound_create *)
        GetProcAddress(DSoundLibrary, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if (!DirectSoundCreate || FAILED(DirectSoundCreate(0, &DirectSound, 0)))
    {
        return;
    }

    WAVEFORMATEX WaveFormat = {};
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = 2;
    WaveFormat.nSamplesPerSec = SamplesPerSecond;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
    WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
    WaveFormat.cbSize = 0;

    if (FAILED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
    {
        return;
    }

    DSBUFFERDESC BufferDescription = {};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

    LPDIRECTSOUNDBUFFER PrimaryBuffer;
    if (FAILED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
    {
        return;
    }

    if (FAILED(PrimaryBuffer->SetFormat(&WaveFormat)))
    {
        return;
    }

    BufferDescription = {};
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
    BufferDescription.dwBufferBytes = BufferSize;
    BufferDescription.lpwfxFormat = &WaveFormat;
    if (FAILED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
    {
        return;
    }

    DSBCAPS Caps = {sizeof(DSBCAPS)};
    if(FAILED(GlobalSecondaryBuffer->GetCaps(&Caps)))
    {
        return;
    }

    if (Caps.dwFlags & DSBCAPS_GETCURRENTPOSITION2)
    {
        // PlayCursor is accurate
    }

    Win32_ClearBuffer();
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
}

static void
Win32_AudioUpdate()
{
    DWORD PlayCursor;
    DWORD WriteCursor;
    if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
    {
        if ((WriteCursor <= PlayCursor && (GlobalSoundOutput.Cursor < WriteCursor || PlayCursor < GlobalSoundOutput.Cursor)) ||
            (WriteCursor >= PlayCursor && (GlobalSoundOutput.Cursor < WriteCursor && PlayCursor < GlobalSoundOutput.Cursor)))
        {
            GlobalSoundOutput.Cursor = WriteCursor;
            Win32_ClearBuffer();
        }

        DWORD BytesToWrite;
        if (GlobalSoundOutput.Cursor <= PlayCursor)
            BytesToWrite = PlayCursor - GlobalSoundOutput.Cursor;
        else
            BytesToWrite = PlayCursor + (GlobalSoundOutput.SecondaryBufferSize - 
                                         GlobalSoundOutput.Cursor);
        
        VOID *Region1; DWORD Region1Size;
        VOID *Region2; DWORD Region2Size;
        if (SUCCEEDED(GlobalSecondaryBuffer->Lock(GlobalSoundOutput.Cursor, BytesToWrite,
                                                  &Region1, &Region1Size,
                                                  &Region2, &Region2Size, 0)))
        {
            game_sound_output SoundBuffer = {};
            SoundBuffer.SamplesPerSecond = GlobalSoundOutput.SamplesPerSecond;
            SoundBuffer.SampleCount = Region1Size / GlobalSoundOutput.BytesPerSample;
            SoundBuffer.Samples = (i16 *)Region1;
            Memory_Set(Region1, 0, Region1Size);
            Audio_OutputSamples(&SoundBuffer);

            if (Region2 && Region2Size)
            {
                SoundBuffer.SampleCount = Region2Size / GlobalSoundOutput.BytesPerSample;
                SoundBuffer.Samples = (i16 *)Region2;
                Memory_Set(Region2, 0, Region2Size);
                Audio_OutputSamples(&SoundBuffer);
            }

            GlobalSoundOutput.Cursor += (Region1Size + Region2Size);
            GlobalSoundOutput.Cursor %= GlobalSoundOutput.SecondaryBufferSize;

            GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
        }
    }
}

static void
Win32_LoadXInput()    
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState) {XInputSetState = XInputSetStateStub;}
    }
    else
    {
        // TODO: Diagnostic
    }
}

static r32
Win32_UpdateStickValue(SHORT Value, SHORT Deadzone)
{
    if (Value < -Deadzone)
        return (r32)((Value + Deadzone) / (32768.f - Deadzone));
    else if (Value > Deadzone)
        return (r32)((Value - Deadzone) / (32767.f - Deadzone));
    return 0.0f;
}

static void
Win32_UpdateButton(u8 Player, button_bits Button, b8 IsDown)
{
    Pico.Hardware.Controllers[Player] &= ~(1 << Button);
    Pico.Hardware.Controllers[Player] |= ((!!IsDown) << Button);
}

static void
Win32_UpdateKey(u8 Player, button_bits Button, b8 IsDown)
{
    Pico.Hardware.Keyboards[Player] &= ~(1 << Button);
    Pico.Hardware.Keyboards[Player] |= ((!!IsDown) << Button);
}

struct win32_window_dimension
{
    i32 Width;
    i32 Height;
};

static win32_window_dimension
Win32_GetWindowDimension(HWND Window)
{
    win32_window_dimension Result;
    
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

static void
Win32_ToggleFullscreen(HWND Window)
{
    //see http://blogs.msdn.com/b/oldnewthing/archive/2010/04/12/9994016.aspx
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if (Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if (GetWindowPlacement(Window, &GlobalWindowPosition) && 
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

static void
Win32_DisplayBufferInWindow(HDC DeviceContext, HWND Window)
{
    win32_window_dimension Dimension = Win32_GetWindowDimension(Window);
    i32 Width = RenderBuffer.Width * (Dimension.Width / RenderBuffer.Width);
    i32 Height = RenderBuffer.Height * (Dimension.Height / RenderBuffer.Height);
    i32 Size = (Width < Height) ? Width : Height;
    i32 Left = (Dimension.Width - Size) / 2;
    i32 Top = (Dimension.Height - Size) / 2;

    StretchDIBits(DeviceContext, Left, Top, Size, Size,
                  0, 0, RenderBuffer.Width, RenderBuffer.Height,
                  RenderBuffer.Pixels, &GlobalBackbufferInfo,
                  DIB_RGB_COLORS, SRCCOPY);

    StretchDIBits(DeviceContext, 0, 0, Dimension.Width, Top, 
                                 0, 0, 0, 0, 0, 0, DIB_RGB_COLORS, BLACKNESS);
    StretchDIBits(DeviceContext, 0, Top, Left, Size, 
                                 0, 0, 0, 0, 0, 0, DIB_RGB_COLORS, BLACKNESS);
    StretchDIBits(DeviceContext, 0, Size + Top, Dimension.Width, Top, 
                                 0, 0, 0, 0, 0, 0, DIB_RGB_COLORS, BLACKNESS);
    StretchDIBits(DeviceContext, Size + Left, Top, Left, Size, 
                                 0, 0, 0, 0, 0, 0, DIB_RGB_COLORS, BLACKNESS);
}

#define RGBtoU32(R, G, B) ((B) | (G) << 8 | (R) << 16 | 256 << 24)
const u32 PicoPalette[16] = { RGBtoU32(  0,   0,   0), RGBtoU32( 29,  43,  83), 
                              RGBtoU32(126,  37,  83), RGBtoU32(  0, 135,  81), 
                              RGBtoU32(171,  82,  54), RGBtoU32( 95,  87,  79), 
                              RGBtoU32(194, 195, 199), RGBtoU32(255, 241, 232),
                              RGBtoU32(255,   0,  77), RGBtoU32(255, 163,   0), 
                              RGBtoU32(255, 236,  39), RGBtoU32(  0, 228,  54),
                              RGBtoU32( 41, 173, 255), RGBtoU32(131, 118, 156), 
                              RGBtoU32(255, 119, 168), RGBtoU32(255, 204, 170) };

static void
Win32_BlitPicoScreen()
{
    for (i32 Y = 0; Y < SCREEN_HEIGHT; ++Y)
    {
        for (i32 X = 0; X < SCREEN_WIDTH / 2; ++X)
        {
            u8 PixelHi = Pico.Screen[Y * SCREEN_WIDTH / 2 + X] >> 4;
            u8 PixelLo = Pico.Screen[Y * SCREEN_WIDTH / 2 + X] & 15;

            PixelHi = Pico.Draw.PaletteScreen[PixelHi] & 15;
            PixelLo = Pico.Draw.PaletteScreen[PixelLo] & 15;

            i32 P = (SCREEN_WIDTH - 1 - Y)  * SCREEN_WIDTH + (X * 2);
            ((u32 *)RenderBuffer.Pixels)[P] = PicoPalette[PixelHi];
            ((u32 *)RenderBuffer.Pixels)[P + 1] = PicoPalette[PixelLo];
        }
    }
}

inline u64
Win32_GetWallClock(void)
{    
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result.QuadPart;
}

inline r32
Win32_GetSecondsElapsed(u64 Start, u64 End)
{
    r32 Result = ((r32)(End - Start) / (r32)GlobalPerfCountFrequency);
    return Result;
}

static void
Win32_CollectInput()
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                u32 VKCode = (u32)Message.wParam;
                
                b8 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b8 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if (WasDown != IsDown)
                {
                    switch (VKCode)
                    {
                        case 'Z': { Win32_UpdateKey(0, BUTTON_BIT_CIRCLE, IsDown); } break;
                        case 'X': { Win32_UpdateKey(0, BUTTON_BIT_CROSS,  IsDown); } break;
                        case 'C': { Win32_UpdateKey(0, BUTTON_BIT_CIRCLE, IsDown); } break;
                        case 'V': { Win32_UpdateKey(0, BUTTON_BIT_CROSS,  IsDown); } break;
                        case 'N': { Win32_UpdateKey(0, BUTTON_BIT_CIRCLE, IsDown); } break;
                        case 'M': { Win32_UpdateKey(0, BUTTON_BIT_CROSS,  IsDown); } break;

                        case VK_UP:    { Win32_UpdateKey(0, BUTTON_BIT_UP,    IsDown); } break;
                        case VK_DOWN:  { Win32_UpdateKey(0, BUTTON_BIT_DOWN,  IsDown); } break;
                        case VK_LEFT:  { Win32_UpdateKey(0, BUTTON_BIT_LEFT,  IsDown); } break;
                        case VK_RIGHT: { Win32_UpdateKey(0, BUTTON_BIT_RIGHT, IsDown); } break;

                        case VK_SHIFT: { Win32_UpdateKey(1, BUTTON_BIT_CIRCLE, IsDown); } break;
                        case 'A':      { Win32_UpdateKey(1, BUTTON_BIT_CROSS,  IsDown); } break;
                        case VK_TAB:   { Win32_UpdateKey(1, BUTTON_BIT_CIRCLE, IsDown); } break;
                        case 'Q':      { Win32_UpdateKey(1, BUTTON_BIT_CROSS,  IsDown); } break;

                        case 'E': { Win32_UpdateKey(1, BUTTON_BIT_UP,    IsDown); } break;
                        case 'D': { Win32_UpdateKey(1, BUTTON_BIT_DOWN,  IsDown); } break;
                        case 'S': { Win32_UpdateKey(1, BUTTON_BIT_LEFT,  IsDown); } break;
                        case 'F': { Win32_UpdateKey(1, BUTTON_BIT_RIGHT, IsDown); } break;
                    }

                    if (IsDown)
                    {
                        b8 AltKeyWasDown = (Message.lParam & (1 << 29)) != 0;
                        if ((VKCode == VK_F4) && AltKeyWasDown)
                        {
                            GlobalRunning = false;;
                        }
                        if ((VKCode == VK_RETURN) && AltKeyWasDown)
                        {
                            if (Message.hwnd)
                                Win32_ToggleFullscreen(Message.hwnd);
                        }
                    }
                }
            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }

    Pico.Hardware.Controllers[0] = 0;
    Pico.Hardware.Controllers[1] = 0;
    XINPUT_STATE ControllerState;
    for (u8 P = 0; P < 2; ++P)
    {
        if (XInputGetState(P, &ControllerState) == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            r32 StickPositionX = Win32_UpdateStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            r32 StickPositionY = Win32_UpdateStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            
            b8 Analog = false;
            if ((StickPositionX != 0.0f) || (StickPositionY != 0.0f))
                Analog = true;

            r32 Threshold = 0.2f;
            if (Analog)
            {
                if (StickPositionY > Threshold) Win32_UpdateButton(P, BUTTON_BIT_UP,    1);
                if (StickPositionY <-Threshold) Win32_UpdateButton(P, BUTTON_BIT_DOWN,  1);
                if (StickPositionX <-Threshold) Win32_UpdateButton(P, BUTTON_BIT_LEFT,  1);
                if (StickPositionX > Threshold) Win32_UpdateButton(P, BUTTON_BIT_RIGHT, 1);
            }
            else
            {
                if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)    != 0) Win32_UpdateButton(P, BUTTON_BIT_UP,    1);
                if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)  != 0) Win32_UpdateButton(P, BUTTON_BIT_DOWN,  1);
                if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)  != 0) Win32_UpdateButton(P, BUTTON_BIT_LEFT,  1);
                if ((Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) Win32_UpdateButton(P, BUTTON_BIT_RIGHT, 1);
            }

            if ((Pad->wButtons & XINPUT_GAMEPAD_Y) != 0)              Win32_UpdateButton(P, BUTTON_BIT_CIRCLE, 1);
            if ((Pad->wButtons & XINPUT_GAMEPAD_A) != 0)              Win32_UpdateButton(P, BUTTON_BIT_CIRCLE, 1);
            if ((Pad->wButtons & XINPUT_GAMEPAD_X) != 0)              Win32_UpdateButton(P, BUTTON_BIT_CROSS, 1);
            if ((Pad->wButtons & XINPUT_GAMEPAD_B) != 0)              Win32_UpdateButton(P, BUTTON_BIT_CROSS, 1);
            if ((Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0)  Win32_UpdateButton(P, BUTTON_BIT_CIRCLE, 1);
            if ((Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) Win32_UpdateButton(P, BUTTON_BIT_CROSS, 1);

            //Win32_UpdateButton(BUTTON_BIT_CIRCLE, (Pad->wButtons & XINPUT_GAMEPAD_START) != 0);
            //Win32_UpdateButton(BUTTON_BIT_CIRCLE, (Pad->wButtons & XINPUT_GAMEPAD_BACK) != 0);
        }
    }
}

static void
Win32_Flip(b8 CalledFromLUA = false)
{
    if (GlobalFocus)
    {
        Win32_BlitPicoScreen();
        HDC DeviceContext = GetDC(GlobalWindow);
        Win32_DisplayBufferInWindow(DeviceContext, GlobalWindow);
        ReleaseDC(GlobalWindow, DeviceContext);
        Win32_AudioUpdate();
    }

    u64 WorkCounter = Win32_GetWallClock();
    r32 WorkSecondsElapsed = Win32_GetSecondsElapsed(GlobalLastCounter, WorkCounter);

    r32 SecondsElapsedForFrame = WorkSecondsElapsed;
    if (SecondsElapsedForFrame < TargetSecondsPerFrame)
    {
        DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                           SecondsElapsedForFrame));
        if (SleepMS > 1)
        {
            Sleep(SleepMS - 1);
        }

        while (SecondsElapsedForFrame < TargetSecondsPerFrame)
        {                            
            SecondsElapsedForFrame = Win32_GetSecondsElapsed(GlobalLastCounter, Win32_GetWallClock());
        }
    }
    else
    {
        // TODO: MISSED FRAME RATE!
    }

    u64 EndCounter = Win32_GetWallClock();
    r32 MSPerFrame = 1000.0f * Win32_GetSecondsElapsed(GlobalLastCounter, EndCounter);
    GlobalLastCounter = EndCounter;

    Win32_CollectInput();

    if (CalledFromLUA && !GlobalRunning)
        exit(0);
}

static LRESULT CALLBACK
Win32_WindowCallback(HWND Window,
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{       
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            Win32_DisplayBufferInWindow(DeviceContext, Window);
            EndPaint(Window, &Paint);
        } break;

        case WM_GETMINMAXINFO:
        {
            MINMAXINFO *MinMaxInfo = (MINMAXINFO *)LParam;

            RECT MinWindowDimension = {};
            MinWindowDimension.right = SCREEN_WIDTH;
            MinWindowDimension.bottom = SCREEN_HEIGHT;
            AdjustWindowRect(&MinWindowDimension, WS_OVERLAPPEDWINDOW, 0);

            POINT p = {MinWindowDimension.right - MinWindowDimension.left,
                       MinWindowDimension.bottom - MinWindowDimension.top};

            MinMaxInfo->ptMinTrackSize = p;
        } break;

        case WM_ENTERSIZEMOVE:
        case WM_KILLFOCUS:
        {
            GlobalFocus = false;
            if (GlobalSecondaryBuffer) 
                GlobalSecondaryBuffer->Stop();
        } break;

        case WM_EXITSIZEMOVE:
        case WM_SETFOCUS:
        {
            GlobalFocus = true;
            if(GlobalSecondaryBuffer) 
                GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
        } break;

        case WM_SETCURSOR:
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}


int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    //lua_State *L = Win32_LoadCart("vibr.p8");
    lua_State *L = Win32_LoadCart("lemonhunter_60.p8");
    //lua_State *L = Win32_LoadCart("circle.p8");

    Win32_LoadXInput();

    WNDCLASSA WindowClass = {};

    RenderBuffer.Width = SCREEN_WIDTH;
    RenderBuffer.Height = SCREEN_HEIGHT;
    RenderBuffer.BytesPerPixel = SCREEN_BYTES_PER_PIXEL;
    RenderBuffer.Pitch = SCREEN_PITCH;
    RenderBuffer.Pixels = VirtualAlloc(0, (RenderBuffer.Pitch * RenderBuffer.Height), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    GlobalBackbufferInfo.bmiHeader.biSize = sizeof(GlobalBackbufferInfo.bmiHeader);
    GlobalBackbufferInfo.bmiHeader.biWidth = RenderBuffer.Width;
    GlobalBackbufferInfo.bmiHeader.biHeight = RenderBuffer.Height;
    GlobalBackbufferInfo.bmiHeader.biPlanes = 1;
    GlobalBackbufferInfo.bmiHeader.biBitCount = 32;
    GlobalBackbufferInfo.bmiHeader.biCompression = BI_RGB;

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32_WindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = 0;//LoadCursor(0, IDC_ARROW);
    WindowClass.hIcon = LoadIcon(Instance, "APP_ICON");
    WindowClass.lpszClassName = "WinPico";
    if (RegisterClassA(&WindowClass))
    {
        RECT WindowDimension = {};
        WindowDimension.right = SCREEN_WIDTH * 4 + 64;
        WindowDimension.bottom = SCREEN_HEIGHT * 4 + 64;
        AdjustWindowRect(&WindowDimension, WS_OVERLAPPEDWINDOW, 0);

        GlobalWindow =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "win-pico",
                WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                CW_USEDEFAULT, CW_USEDEFAULT,
                WindowDimension.right - WindowDimension.left,
                WindowDimension.bottom - WindowDimension.top,
                0, 0, Instance, 0);

        if (GlobalWindow)
        {
            GlobalRunning = true;

            UINT DesiredSchedulerMS = 1;
            b8 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

            i32 MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(GlobalWindow);
            i32 RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(GlobalWindow, RefreshDC);
            if (RefreshRate > 1)
            {
                MonitorRefreshHz = RefreshRate;
            }
            
            GlobalSoundOutput.Cursor = 0;
            GlobalSoundOutput.SamplesPerSecond = 22050;
            GlobalSoundOutput.BytesPerSample = sizeof(i16) * 2;
            GlobalSoundOutput.SecondaryBufferSize = (GlobalSoundOutput.SamplesPerSecond * 
                                                     GlobalSoundOutput.BytesPerSample) / 
                                                     ((DWORD)GameUpdateHz / 4);
            Win32_InitDSound(GlobalWindow, GlobalSoundOutput.SamplesPerSecond, GlobalSoundOutput.SecondaryBufferSize);
            

            LARGE_INTEGER PerfCountFrequencyResult;
            QueryPerformanceFrequency(&PerfCountFrequencyResult);
            GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

            GlobalLastCounter = Win32_GetWallClock();

            while (GlobalRunning)
            {
                if (GlobalFocus)
                {
                    // UPDATE
                    Pico_Update(L);

                    // RENDER
                    Pico_Draw(L);
                }

                Win32_Flip();
            }
        }
    }

    return 0;
}