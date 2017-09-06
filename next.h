//----------------------------------------------------------------------------------------------------------------------
// ZX Spectrum Next mock
// Copyright (C)2017 Mattie Davies.
//
// This interface allows you to mock the IO ports, VRAM and DOS calls that the real next.  This allows you to prototype
// your game quickly in C/C++ on your PC.  Later you can rewrite your game in Z80.
//
// Current features implemented:
//
//      - 4 zoom modes
//      - Original 48K ULA (including border)
//      - 512K Memory Map (32 pages).
//      - Layer 2.
//
// Future features planned to be implemented:
//
//      - Keyboard support.
//      - Kempston support (joystick and mouse).
//      - Full Next video support (including ULAnext, sprites & priorities).
//      - Full RAM bank switching.
//      - AY3-8912 support.
//      - SID support.
//
// Include this file when you need to use the API, but it must be included at least once in your code base with
// NX_IMPLEMENTATION defined otherwise you will get linker errors as the linker will not find the implementation of
// the API.
//
// Some keys have functionality:
//
//      ESC     Quit the current window
//      F1      Zoom 100%
//      F2      Zoom 200%
//      F3      Zoom 300%
//      F4      Zoom 400%
//      F5      Output a screenshot (writes to current directory with name "NextImage#", where # is an increasing number
//
//----------------------------------------------------------------------------------------------------------------------

#pragma once

#include <stdint.h>

//----------------------------------------------------------------------------------------------------------------------
// Type definitions
//----------------------------------------------------------------------------------------------------------------------

typedef int8_t      nxSignedByte;
typedef int16_t     nxSignedWord;
typedef int32_t     nxSignedDword;
typedef int64_t     nxInt;

typedef uint8_t     nxByte;
typedef uint16_t    nxWord;
typedef uint32_t    nxDword;
typedef uint64_t    nxQword;

typedef double      nxFloat;

typedef char        nxBool;

#define NX_YES (1)
#define NX_NO (0)
#define NX_AS_BOOL(x) ((x) ? NX_YES : NX_NO)

// Use on nxWord types to fetch the high and low bytes
#define NX_HI(x) ((nxByte)((x) / 256))
#define NX_LO(x) ((nxByte)((x) % 256))

//----------------------------------------------------------------------------------------------------------------------
// API for the system
//
// This API will manage the Next Mock system.  Typical use is:
//
//      Next N = nxOpen();
//
//      ... Initialise memory here using the memory API ...
//
//      while(nxUpdate())
//      {
//          ... Run your code ...
//
//          // Call redraw to update the OS windows with the data in VRAM.
//          nxRedraw(N);
//      }
//
//      nxClose(N);
//
//----------------------------------------------------------------------------------------------------------------------

typedef struct _Next* Next;
typedef void(*NxFrameRoutine)(Next);

// Implement this routines as your entry routine.
int nxMain(int argc, char** argv);

// Create a new Next context.  Will act on the command line parameters it knows, and ignore the others
Next nxOpen();

// Destroy the context
void nxClose(Next N);

// Update the message pump to the windows.  This will interface with the OS and should be called as much as possible.
// Interleave calls here with your own code.  This will return NX_NO, when there are no more windows open.  Every
// beginning of the frame, an optional function is called before the screen is redrawn.  The screen is only redrawn
// if nxRedraw() is called or if 16 frames have passed (to update the flash state).
nxBool nxUpdate(Next N, NxFrameRoutine f);

// Open a console for logging.  "printf"s will be forwarded to this console.  It supports ANSI colour codes if that's
// your thing.
void nxConsoleOpen();

// Single the window to be redrawn on the next nxUpdate().  Will not actually redraw if nothing visual has changed.
void nxRedraw(Next N);

//----------------------------------------------------------------------------------------------------------------------
// Memory API
//
// This API can only see 64K.  Any writes past 64K will wrap around, just like the real hardware.  Mass writing
// functions like nxPokeBuffer and nxPokeFile will not wrap around, but rather error if there is too much data.
//
//----------------------------------------------------------------------------------------------------------------------

// Write a byte to the current mapped memory.
void nxPoke(Next N, nxWord address, nxByte b);

// Write a 16-bit value, little endian, to the current mapped memory.
void nxPoke16(Next N, nxWord address, nxWord b);

// Write memory contents to the address.  Will return NX_NO if the contents are too big.
nxBool nxPokeBuffer(Next N, nxWord address, const void* buffer, nxWord size);

// Utility function that opens a file, reads it in, calls nxPokeBuffer, then closes the file.
nxBool nxPokeFile(Next N, nxWord address, const char* fileName);

// Read a byte from the current mapped memory.
nxByte nxPeek(Next N, nxWord address);

// Read a 16-bit value, little endian, from the current mapped memory.
nxWord nxPeek16(Next N, nxWord address);

// Read directly from a bank.
nxByte nxPeekEx(Next N, nxWord bank, nxWord p);

// Read a 16-bit value directly from a bank.
nxWord nxPeek16Ex(Next N, nxWord bank, nxWord p);

//----------------------------------------------------------------------------------------------------------------------
// IO Ports API
//----------------------------------------------------------------------------------------------------------------------

// OUT:
//   7   6   5   4   3   2   1   0
// +---+---+---+---+---+-----------+
// |   |   |   | E | M |   Border  |
// +---+---+---+---+---+-----------+
//
// IN:
//  High byte set except for keyboard rows that you want to read:
//
//      Binary:     Hex:    0       1       2       3       4
//      ---------   ----    ------  ------  ------  ------  ------
//      1111 1110   FE      Shift   Z       X       C       V
//      1111 1101   FD      A       S       D       F       G
//      1111 1011   FB      Q       W       E       R       T
//      1111 0111   F7      1       2       3       4       5
//      1110 1111   EF      0       9       8       7       6
//      1101 1111   DF      P       O       I       U       Y
//      1011 1111   BF      Enter   L       K       J       H
//      0111 1111   7f      Space   Sym     M       N       B
//
//  Bit is 0, then key is pressed.  If multiple rows are read, then bits are the AND of the values, i.e. 1 is all keys
//  for that position on read rows are not pressed, 0 is one of the keys for that position on read rows is pressed.
//
//  Bit 5: 1
//  Bit 6: EAR
//  Bit 7: 1
//
#define NX_PORT_ULA             0x00fe

// NEXT ports
//
#define NX_PORT_LAYER2_PAGING   0x123b
#define NX_PORT_REG_SELECT      0x243b
#define NX_PORT_REG_RW          0x253b

// Output a byte to a port address
void nxOut(Next N, nxWord port, nxByte b);

// Read a byte from a port address
nxByte nxIn(Next N, nxWord port);

// Convenience function for Next registers
void nxWriteReg(Next N, nxByte reg, nxByte value);
nxByte nxReadReg(Next N, nxByte reg);

//----------------------------------------------------------------------------------------------------------------------
// File loading
// These routines allow reading and writing to and from files using memory mapping.  These are used internally for
// PNG writes and file loading into memory, but are exposed for convenience.
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    nxByte*     bytes;
    nxInt       size;

    void*       file;
    void*       fileMap;
}
NxData;


// Load a file into memory and return a structure describing the data.  NxData.bytes will point to the buffer and
// NxData.size will be the 64-bit length.  The file is open (shared for reading) until nxDataUnload is called.
NxData nxDataLoad(const char* fileName);

// Unload a file and release the memory used.  The file is closed.
void nxDataUnload(NxData d);

// Create a new file with the given size.  Write your data to the buffer described by NxData and call nxDataUnload
// to write the file.
NxData nxDataMake(const char* fileName, nxInt size);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPLEMENTATION

#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NxInternal static

//----------------------------------------------------------------------------------------------------------------------
// Basic memory allocation
//----------------------------------------------------------------------------------------------------------------------

NxInternal void* nxMemoryOp(void* oldAddress, nxInt oldNumBytes, nxInt newNumBytes, const char* file, int line)
{
    void* p = 0;

    if (newNumBytes)
    {
        p = realloc(oldAddress, newNumBytes);
    }
    else
    {
        free(oldAddress);
    }

    return p;
}

NxInternal void* nxMemoryAlloc(nxInt numBytes, const char* file, int line)
{
    return nxMemoryOp(0, 0, numBytes, file, line);
}

NxInternal void* nxMemoryRealloc(void* address, nxInt oldNumBytes, nxInt newNumBytes, const char* file, int line)
{
    return nxMemoryOp(address, oldNumBytes, newNumBytes, file, line);
}

NxInternal void nxMemoryFree(void* address, nxInt numBytes, const char* file, int line)
{
    nxMemoryOp(address, numBytes, 0, file, line);
}

NxInternal void nxMemoryCopy(const void* src, void* dst, nxInt numBytes)
{
    memcpy(dst, src, (size_t)numBytes);
}

NxInternal void nxMemoryMove(const void* src, void* dst, nxInt numBytes)
{
    memmove(dst, src, (size_t)numBytes);
}

NxInternal void nxMemoryClear(void* mem, nxInt numBytes)
{
    memset(mem, 0, (size_t)numBytes);
}

#define NX_ALLOC(numBytes) nxMemoryAlloc((numBytes), __FILE__, __LINE__)
#define NX_REALLOC(address, oldNumBytes, newNumBytes) nxMemoryRealloc((address), (oldNumBytes), (newNumBytes), __FILE__, __LINE__)
#define NX_FREE(address, oldNumBytes) nxMemoryFree((address), (oldNumBytes), __FILE__, __LINE__)


//----------------------------------------------------------------------------------------------------------------------
// C-based arrays
//----------------------------------------------------------------------------------------------------------------------

#define NxArray(T) T*

#define NX_ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

// Destroy an array
#define nxArrayRelease(a) ((a) ? NX_FREE((nxByte *)a - (sizeof(nxInt) * 2), (sizeof(*a) * __nxArrayCapacity(a)) + (sizeof(nxInt) * 2)), 0 : 0)

// Add an element to the end of an array
#define nxArrayAdd(a, v) (__nxArrayMayGrow(a, 1), (a)[__nxArrayCount(a)++] = (v))

// Return the number of elements in an array
#define nxArrayCount(a) ((a) ? __nxArrayCount(a) : 0)

// Add n uninitialised elements to the array
#define nxArrayExpand(a, n) (__nxArrayMayGrow(a, n), __nxArrayCount(a) += (n), &(a)[__nxArrayCount(a) - (n)])

// Reserve capacity for n extra items to the array
#define nxArrayReserve(a, n) (__nxArrayMayGrow(a, n))

// Clear the array
#define nxArrayClear(a) (arrayCount(a) = 0)

// Delete an array entry
#define nxArrayDelete(a, i) (memoryMove(&(a)[(i)+1], &(a)[(i)], (__nxArrayCount(a) - (i) - 1) * sizeof(*a)), --__nxArrayCount(a), (a))

//
// Internal routines
//

#define __nxArrayRaw(a) ((nxInt *)(a) - 2)
#define __nxArrayCount(a) __nxArrayRaw(a)[1]
#define __nxArrayCapacity(a) __nxArrayRaw(a)[0]

#define __nxArrayNeedsToGrow(a, n) ((a) == 0 || __nxArrayCount(a) + (n) >= __nxArrayCapacity(a))
#define __nxArrayMayGrow(a, n) (__nxArrayNeedsToGrow(a, (n)) ? __nxArrayGrow(a, n) : 0)
#define __nxArrayGrow(a, n) ((a) = __nxArrayInternalGrow((a), (n), sizeof(*(a))))

NxInternal void* __nxArrayInternalGrow(void* a, nxInt increment, nxInt elemSize)
{
    nxInt doubleCurrent = a ? 2 * __nxArrayCapacity(a) : 0;
    nxInt minNeeded = nxArrayCount(a) + increment;
    nxInt capacity = doubleCurrent > minNeeded ? doubleCurrent : minNeeded;
    nxInt oldBytes = a ? elemSize * nxArrayCount(a) + sizeof(nxInt) * 2 : 0;
    nxInt bytes = elemSize * capacity + sizeof(nxInt) * 2;
    nxInt* p = (nxInt *)NX_REALLOC(a ? __nxArrayRaw(a) : 0, oldBytes, bytes);
    if (p)
    {
        if (!a) p[1] = 0;
        p[0] = capacity;
        return p + 2;
    }
    else
    {
        return 0;
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Global variables (YES I KNOW!) and constants
//----------------------------------------------------------------------------------------------------------------------

int gArgc = 0;
char** gArgv = 0;

typedef struct _WindowInfo WindowInfo;
typedef int Window;

NxArray(WindowInfo) gWindows = 0;
int gWindowRefCount = 0;

#define NX_SCREEN_WIDTH     256
#define NX_SCREEN_HEIGHT    192
#define NX_WINDOW_WIDTH     320
#define NX_WINDOW_HEIGHT    256
#define NX_BORDER_WIDTH     ((NX_WINDOW_WIDTH - NX_SCREEN_WIDTH) / 2)
#define NX_BORDER_HEIGHT    ((NX_WINDOW_HEIGHT - NX_SCREEN_HEIGHT) / 2)
#define NX_NUM_PAGES        32

struct _Next
{
    Window              window;
    nxDword*            image;

    // Memory
    nxByte              banks[4];
    nxByte              pages[NX_NUM_PAGES][16384];
    nxByte              palette[256];

    // Timings
    clock_t             lastTimeStamp;  // Used to measure time passing
    nxFloat             currentTime;    // Used to know when interrupt is passing
    int                 flashCount;
    nxBool              flash;

    // IO state
    nxByte              border;

    // Layer-2 state
    nxByte              layer2Bank;                 // Sub bank (0-2) of layer
    nxByte              layer2BankStart;            // Start bank for layer 2 VRAM
    nxByte              layer2ShadowBankStart;      // Start bank for layer 2 shadow VRAM
    nxBool              layer2ShadowEnable;         // Select for shadow VRAM
    nxBool              layer2Enable;               // Layer 2 visible
    nxBool              layer2Write0;               // RAM slot 1 mapped to VRAM (shadow or normal)

    // Next registers
    nxByte              nextRegSelect;
};

//----------------------------------------------------------------------------------------------------------------------
// Rendering
//----------------------------------------------------------------------------------------------------------------------

NxInternal void nxRenderULA(Next N)
{
    static const nxDword colours[16] =
    {
        0x000000, 0x0000d7, 0xd70000, 0xd700d7, 0x00d700, 0x00d7d7, 0xd7d700, 0xd7d7d7,
        0x000000, 0x0000ff, 0xff0000, 0xff00ff, 0x00ff00, 0x00ffff, 0xffff00, 0xffffff,
    };

    nxDword* img = N->image;
    int border = 7;

    for (int r = -NX_BORDER_HEIGHT; r < (NX_SCREEN_HEIGHT + NX_BORDER_HEIGHT); ++r)
    {
        if (r < 0 || r >= NX_SCREEN_HEIGHT)
        {
            for (int c = -NX_BORDER_WIDTH; c < (NX_SCREEN_WIDTH + NX_BORDER_WIDTH); ++c)
            {
                *img++ = colours[N->border & 0x7];
            }
        }
        else
        {
            // Video data.
            //  Pixels address is 010S SRRR CCCX XXXX
            //  Attrs address is 0101 10YY YYYX XXXX
            //  S = Section (0-2)
            //  C = Cell row within section (0-7)
            //  R = Pixel row within cell (0-7)
            //  X = X coord (0-31)
            //  Y = Y coord (0-23)
            //
            //  ROW = SSCC CRRR
            //      = YYYY Y000
            nxWord bank = 5;
            nxWord p = ((r & 0x0c0) << 5) + ((r & 0x7) << 8) + ((r & 0x38) << 2);
            nxWord a = 0x1800 + ((r & 0xf8) << 2);

            for (int c = -NX_BORDER_WIDTH; c < (NX_SCREEN_WIDTH + NX_BORDER_WIDTH); ++c)
            {
                if (c < 0 || c >= NX_SCREEN_WIDTH)
                {
                    *img++ = colours[N->border & 0x7];
                }
                else
                {
                    nxByte data = nxPeekEx(N, bank, p++);
                    nxByte attr = nxPeekEx(N, bank, a++);
                    nxDword ink = colours[(attr & 7) + ((attr & 0x40) >> 3)];
                    nxDword paper = colours[(attr & 0x7f) >> 3];
                    nxBool flash = NX_AS_BOOL(attr & 0x80);

                    if (flash && N->flash)
                    {
                        nxDword t = ink;
                        ink = paper;
                        paper = t;
                    }

                    for (int i = 7; i >= 0; --i)
                    {
                        img[i] = (data & 1) ? ink : paper;
                        data >>= 1;
                    }
                    img += 8;
                    c += 7;  // for loop with increment it once more
                }
            }
        }
    }
}

static nxDword kColour_3bit[] = { 0, 36, 73, 109, 146, 182, 219, 255 };
static nxDword kColour_2bit[] = { 0, 85, 170, 255 };

NxInternal nxDword nxConvertNextLayer2Pixel(Next N, nxByte pixel)
{
    pixel = N->palette[pixel];
    nxDword r = (nxDword)((pixel & 0xe0) >> 5);
    nxDword g = (nxDword)((pixel & 0x1c) >> 2);
    nxDword b = (nxDword)(pixel & 0x03);
    return (nxDword)0xff000000 + (kColour_3bit[r] << 16) + (kColour_3bit[g] << 8) + kColour_2bit[b];
}

void nxRenderLayer2(Next N)
{
    nxDword* img = N->image + NX_BORDER_WIDTH + (NX_BORDER_HEIGHT * NX_WINDOW_WIDTH);
    nxByte bank = N->layer2ShadowEnable ? N->layer2ShadowBankStart : N->layer2BankStart;

    for (int b = 0; b < 3; ++b)
    {
        nxWord address = 0;
        for (int row = 0; row < 64; ++row)
        {
            for (int col = 0; col < 256; ++col)
            {
                nxByte pixel = nxPeekEx(N, bank + b, address++);
                if (pixel != 0xe3)
                {
                    img[col] = nxConvertNextLayer2Pixel(N, pixel);
                }
            }
            img += NX_WINDOW_WIDTH;
        }
    }
}

NxInternal void nxRender(Next N)
{
    nxRenderULA(N);
    if (N->layer2Enable)
    {
        nxRenderLayer2(N);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Windows operations
//----------------------------------------------------------------------------------------------------------------------

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct _WindowInfo
{
    Next            N;
    HWND            handle;
    BITMAPINFO      info;
    int             imageWidth;
    int             imageHeight;
    int             windowWidth;
    int             windowHeight;
};

ATOM gWindowClassAtom = 0;

NxInternal Window nxWin32AllocHandle()
{
    for (int i = 0; i < nxArrayCount(gWindows); ++i)
    {
        if (gWindows[i].handle == 0) return i;
    }

    gWindows = nxArrayExpand(gWindows, 1);
    return (Window)(nxArrayCount(gWindows) - 1);
}

NxInternal Window nxWin32FindHandle(HWND wnd)
{
    nxInt count = nxArrayCount(gWindows);
    for (int i = 0; i < count; ++i)
    {
        if (gWindows[i].handle == wnd) return i;
    }

    return -1;
}

typedef struct WindowCreateInfo
{
    Window handle;
}
WindowCreateInfo;

void nxWin32Lock();
void nxWin32Unlock();
void nxWin32CloseWindow(Window window);
Window nxWin32MakeWindow(const char* title, Next N, int scale);

NxInternal LRESULT CALLBACK nxWin32WindowProc(HWND wnd, UINT msg, WPARAM w, LPARAM l)
{
    if (msg == WM_CREATE)
    {
        CREATESTRUCTA* cs = (CREATESTRUCTA *)l;
        WindowCreateInfo* wci = (WindowCreateInfo *)cs->lpCreateParams;
        gWindows[wci->handle].handle = wnd;
    }
    else
    {
        Window window = nxWin32FindHandle(wnd);
        WindowInfo* info = (window == -1 ? 0 : &gWindows[window]);

        switch (msg)
        {
        case WM_SIZE:
            if (info)
            {
                int bitmapMemorySize = 0;

                info->windowWidth = LOWORD(l);
                info->windowHeight = HIWORD(l);

                ZeroMemory(&info->info.bmiHeader, sizeof(info->info.bmiHeader));

                info->info.bmiHeader.biSize = sizeof(info->info.bmiHeader);
                info->info.bmiHeader.biWidth = info->imageWidth;
                info->info.bmiHeader.biHeight = -info->imageHeight;
                info->info.bmiHeader.biPlanes = 1;
                info->info.bmiHeader.biBitCount = 32;
                info->info.bmiHeader.biClrImportant = BI_RGB;
            }
            break;

        case WM_PAINT:
            if (info)
            {
                nxRender(info->N);
                PAINTSTRUCT ps;
                HDC dc = BeginPaint(wnd, &ps);
                StretchDIBits(dc,
                    0, 0, info->windowWidth, info->windowHeight,
                    0, 0, info->imageWidth, info->imageHeight,
                    info->N->image, &info->info,
                    DIB_RGB_COLORS, SRCCOPY);
                EndPaint(wnd, &ps);
            }
            break;

        case WM_CLOSE:
            DestroyWindow(wnd);
            break;

        case WM_DESTROY:
            if (--gWindowRefCount == 0)
            {
                PostQuitMessage(0);
            }
            info->handle = INVALID_HANDLE_VALUE;
            break;

        case WM_KEYDOWN:
            {
                int scale = 0;
                switch (w)
                {
                case VK_ESCAPE:
                    PostMessageA(wnd, WM_CLOSE, 0, 0);
                    break;

                case VK_F1:     scale = 1;      break;
                case VK_F2:     scale = 2;      break;
                case VK_F3:     scale = 3;      break;
                case VK_F4:     scale = 4;      break;
                }

                if (scale)
                {
                    int wndWidth = gWindows[info->N->window].imageWidth * scale;
                    int wndHeight = gWindows[info->N->window].imageHeight * scale;
                    gWindows[info->N->window].windowWidth = wndWidth;
                    gWindows[info->N->window].windowHeight = wndHeight;

                    RECT r = { 0, 0, wndWidth, wndHeight };
                    DWORD style = GetWindowLongA(wnd, GWL_STYLE);
                    DWORD exStyle = GetWindowLongA(wnd, GWL_EXSTYLE);
                    AdjustWindowRectEx(&r, style, FALSE, exStyle);

                    SetWindowPos(wnd, 0, 0, 0, r.right - r.left, r.bottom - r.top,
                        SWP_NOMOVE | SWP_NOZORDER);
                }
            }
            break;

        default:
            return DefWindowProcA(wnd, msg, w, l);
        }
    }

    return 0;
}

Window nxWin32MakeWindow(const char* title, Next N, int scale)
{
    WindowCreateInfo wci;
    Window w = nxWin32AllocHandle();
    int width = NX_WINDOW_WIDTH;
    int height = NX_WINDOW_HEIGHT;
    nxDword* img = N->image;

    nxMemoryClear(&gWindows[w], sizeof(WindowInfo));

    wci.handle = w;

    gWindows[w].N = N;
    gWindows[w].handle = 0;
    gWindows[w].imageWidth = width;
    gWindows[w].imageHeight = height;
    gWindows[w].windowWidth = width * scale;
    gWindows[w].windowHeight = width * scale;

    RECT r = { 0, 0, width * scale, height * scale };
    int style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE;

    if (!gWindowClassAtom)
    {
        WNDCLASSEXA wc = { 0 };

        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = &nxWin32WindowProc;
        wc.hInstance = GetModuleHandleA(0);
        wc.hIcon = wc.hIconSm = LoadIconA(0, IDI_APPLICATION);
        wc.hCursor = LoadCursorA(0, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszClassName = "k_bitmap_window";

        gWindowClassAtom = RegisterClassExA(&wc);
    }

    AdjustWindowRect(&r, style, FALSE);

    gWindows[w].handle = CreateWindowA("k_bitmap_window", title, style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        r.right - r.left, r.bottom - r.top,
        0, 0, GetModuleHandleA(0), &wci);

    ++gWindowRefCount;

    return w;
}

void nxWin32CloseWindow(Window window)
{
    SendMessageA(gWindows[window].handle, WM_CLOSE, 0, 0);
}

nxBool nxWin32Pump()
{
    nxBool cont = NX_YES;
    MSG msg;

    while (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE))
    {
        if (!GetMessageA(&msg, 0, 0, 0)) cont = NX_NO;
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return cont;
}

void nxWin32Lock()
{
    ++gWindowRefCount;
}

void nxWin32Unlock()
{
    --gWindowRefCount;
}

void nxWin32Redraw(Window window)
{
    InvalidateRect(gWindows[window].handle, 0, FALSE);
}

void nxDataUnload(NxData d)
{
    if (d.bytes)        UnmapViewOfFile(d.bytes);
    if (d.fileMap)      CloseHandle(d.fileMap);
    if (d.file)         CloseHandle(d.file);

    d.bytes = 0;
    d.size = 0;
    d.file = INVALID_HANDLE_VALUE;
    d.fileMap = INVALID_HANDLE_VALUE;
}

NxData nxDataLoad(const char* fileName)
{
    NxData d = { 0 };

    d.file = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (d.file)
    {
        DWORD fileSizeHigh, fileSizeLow;
        fileSizeLow = GetFileSize(d.file, &fileSizeHigh);
        d.fileMap = CreateFileMappingA(d.file, 0, PAGE_READONLY, fileSizeHigh, fileSizeLow, 0);

        if (d.fileMap)
        {
            d.bytes = MapViewOfFile(d.fileMap, FILE_MAP_READ, 0, 0, 0);
            d.size = ((nxInt)fileSizeHigh << 32) | fileSizeLow;
        }
        else
        {
            nxDataUnload(d);
        }
    }

    return d;
}

NxData nxDataMake(const char* fileName, nxInt size)
{
    NxData d = { 0 };

    d.file = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (d.file)
    {
        DWORD fileSizeLow = (size & 0xffffffff);
        DWORD fileSizeHigh = (size >> 32);
        d.fileMap = CreateFileMappingA(d.file, 0, PAGE_READWRITE, fileSizeHigh, fileSizeLow, 0);

        if (d.fileMap)
        {
            d.bytes = MapViewOfFile(d.fileMap, FILE_MAP_WRITE, 0, 0, 0);
            d.size = size;
        }
        else
        {
            nxDataUnload(d);
        }
    }

    return d;
}

#endif // _WIN32

//----------------------------------------------------------------------------------------------------------------------
// Win32 implementation of API
//----------------------------------------------------------------------------------------------------------------------

#define FRAME_RATE  50
#define FRAME_TIME  (1.0 / (nxFloat)FRAME_RATE)

NxInternal nxFloat nxTime(Next N)
{
    clock_t t = clock();
    clock_t diff = t - N->lastTimeStamp;
    nxFloat secs = (nxFloat)diff / (nxFloat)CLOCKS_PER_SEC;
    N->lastTimeStamp = t;
    return secs;
}

Next nxOpen()
{
    Next N = NX_ALLOC(sizeof(struct _Next));

    nxMemoryClear(N, sizeof(struct _Next));
    N->image = NX_ALLOC(sizeof(nxDword) * NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT);
    N->window = nxWin32MakeWindow("ZX Spectrum Next", N, 4);
    N->flash = NX_NO;
    N->flashCount = 0;
    N->lastTimeStamp = clock();
    N->currentTime = 0;

    N->banks[0] = 0;
    N->banks[1] = 5;
    N->banks[2] = 2;
    N->banks[3] = 0;

    for (int i = 0; i < 256; ++i) N->palette[i] = i;

    N->layer2Bank = 0;
    N->layer2BankStart = 8;
    N->layer2ShadowBankStart = 11;
    N->layer2ShadowEnable = NX_NO;
    N->layer2Enable = NX_NO;
    N->layer2Write0 = NX_NO;

    N->nextRegSelect = 0;

    return N;
}

void nxClose(Next N)
{
    if (N)
    {
        if (gWindows[N->window].handle != INVALID_HANDLE_VALUE)
        {
            nxWin32CloseWindow(N->window);
        }
        NX_FREE(N->image, sizeof(nxDword) * NX_WINDOW_WIDTH * NX_WINDOW_HEIGHT);
        NX_FREE(N, sizeof(struct _Next));
    }
}

nxBool nxUpdate(Next N, NxFrameRoutine f)
{
    nxFloat t = nxTime(N);
    N->currentTime += t;
    if (N->currentTime > FRAME_TIME)
    {
        N->currentTime -= FRAME_TIME;
        if (++N->flashCount == 16)
        {
            N->flashCount = 0;
            N->flash = !N->flash;
            nxRedraw(N);
        }
        if (f)
        {
            f(N);
        }
    }
    return nxWin32Pump();
}

void nxRedraw(Next N)
{
    nxWin32Redraw(N->window);
}

NxInternal BOOL WINAPI nxWin32HandleConsoleClose(DWORD ctrlType)
{
    return TRUE;
}

void nxConsoleOpen()
{
    AllocConsole();
    SetConsoleTitleA("Debug Window");

    HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
    int hCrt = _open_osfhandle((intptr_t)handle_out, _O_TEXT);
    FILE* hf_out = _fdopen(hCrt, "w");
    setvbuf(hf_out, NULL, _IONBF, 1);
    freopen("CONOUT$", "w", stdout);

    HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
    hCrt = _open_osfhandle((intptr_t)handle_in, _O_TEXT);
    FILE* hf_in = _fdopen(hCrt, "r");
    setvbuf(hf_in, NULL, _IONBF, 0);
    freopen("CONIN$", "r", stdin);

    DWORD mode;
    GetConsoleMode(handle_out, &mode);
    mode |= 0x4;
    SetConsoleMode(handle_out, mode);

    SetConsoleCtrlHandler(&nxWin32HandleConsoleClose, TRUE);
    printf("\033[31;1mWarning: \033[0mClosing this window will terminate the application immediately.\n\n");
}

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR cmdLine, int cmdShow)
{
    return nxMain(__argc, __argv);
}

//----------------------------------------------------------------------------------------------------------------------
// Win32 version of memory API
//----------------------------------------------------------------------------------------------------------------------

NxInternal nxCalcMem(Next N, nxWord address, nxWord* bank, nxWord* p, nxBool isWrite)
{
    nxWord slot = (address & 0xc000) >> 14;
    *p = (address & 0x3fff);

    if (isWrite && N->layer2Write0)
    {
        // Slot 1 writes access current VRAM
        *bank = N->layer2ShadowEnable ? N->layer2ShadowBankStart + N->layer2Bank
                                      : N->layer2BankStart + N->layer2Bank;
    }
    else
    {
        *bank = N->banks[slot];
    }
}

void nxPoke(Next N, nxWord address, nxByte b)
{
    nxWord bank, p;
    nxCalcMem(N, address, &bank, &p, NX_YES);
    N->pages[bank][p] = b;
}

void nxPoke16(Next N, nxWord address, nxWord b)
{
    nxPoke(N, address, NX_LO(b));
    nxPoke(N, address+1, NX_HI(b));
}

nxBool nxPokeBuffer(Next N, nxWord address, const void* buffer, nxWord size)
{
    if ((nxInt)address + (nxInt)size > 65536) return NX_NO;
    nxByte* b = (nxByte *)buffer;
    for (nxWord i = 0; i < size; ++i)
    {
        nxPoke(N, address + i, *b++);
    }
    nxRedraw(N);
    return NX_YES;
}

nxBool nxPokeFile(Next N, nxWord address, const char* fileName)
{
    nxBool result = NX_NO;
    NxData d = nxDataLoad(fileName);
    if (d.bytes)
    {
        result = nxPokeBuffer(N, address, d.bytes, (nxWord)d.size);
        nxDataUnload(d);
    }

    return result;
}

nxByte nxPeek(Next N, nxWord address)
{
    nxWord bank, p;
    nxCalcMem(N, address, &bank, &p, NX_NO);
    return nxPeekEx(N, bank, p);
}

nxWord nxPeek16(Next N, nxWord address)
{
    return nxPeek(N, address) + 256 * nxPeek(N, address + 1);
}

nxByte nxPeekEx(Next N, nxWord bank, nxWord p)
{
    p &= 0x3fff;
    return N->pages[bank][p];
}

nxWord nxPeek16Ex(Next N, nxWord bank, nxWord p)
{
    return nxPeekEx(N, bank, p) + 256 * nxPeekEx(N, bank, p + 1);
}

//----------------------------------------------------------------------------------------------------------------------
// IO port API
//----------------------------------------------------------------------------------------------------------------------

void nxOut(Next N, nxWord port, nxByte b)
{
    nxByte h = NX_HI(port);
    nxByte l = NX_LO(port);

    switch (l)
    {
    case 0xfe:
        {
            nxByte border = b & 7;
            N->border = border;
            nxRedraw(N);
        }
        break;

    case 0x3b:
        // Next ports
        switch (h)
        {
        case 0x12:  // Layer 2 access port
            {
                N->layer2Bank = (b & 0xc0) >> 6;
                N->layer2ShadowEnable = NX_AS_BOOL(b & 0x08);
                N->layer2Enable = NX_AS_BOOL(b & 0x02);
                N->layer2Write0 = NX_AS_BOOL(b & 0x01);
                nxRedraw(N);
            }
            break;

        case 0x24:  // Register select
            N->nextRegSelect = b;
            break;

        case 0x25:  // Register read/write
            switch (N->nextRegSelect)
            {
            case 0x12:  // Layer 2 bank start
                N->layer2BankStart = (b & 31);
                nxRedraw(N);
                break;

            case 0x13:  // Layer 2 shadow bank start
                N->layer2ShadowBankStart = (b & 31);
                nxRedraw(N);
                break;
            }
            break;

        case 0x30:  // Sprite status/slot select
            break;
        }
    }
}

nxByte nxIn(Next N, nxWord port)
{
    return 0;
}

void nxWriteReg(Next N, nxByte reg, nxByte value)
{
    nxOut(N, NX_PORT_REG_SELECT, reg);
    nxOut(N, NX_PORT_REG_RW, value);
}

nxByte nxReadReg(Next N, nxByte reg)
{
    nxOut(N, NX_PORT_REG_SELECT, reg);
    return nxIn(N, NX_PORT_REG_RW);
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPLEMENTATION
