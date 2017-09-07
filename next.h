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
//      - 1MB Memory Map (64 pages).
//      - Layer 2.
//      - Full RAM bank switching to $c000
//
// Future features planned to be implemented:
//
//      - Keyboard support.
//      - Kempston support (joystick and mouse).
//      - Full Next video support (including ULAnext, Timex modes, sprites & priorities).
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
// STB Image support.
// This is required to enable PNG reading.  If you require this functionality, define NX_USE_STB when you define
// NX_IMPLEMENTATION
//----------------------------------------------------------------------------------------------------------------------

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
//      while(nxUpdate(N, 0))
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
void nxPoke16(Next N, nxWord address, nxWord w);

// Write a byte to a specific bank.  Address must be from $0000-$3fff
void nxPokeEx(Next N, nxByte bank, nxWord address, nxByte b);

// Write a 16-bit value to a specific bank.  Address must be from $0000-$3fff
void nxPoke16Ex(Next N, nxByte bank, nxWord address, nxWord w);

// Write memory contents to the address.  Will return NX_NO if the contents are too big.
nxBool nxPokeBuffer(Next N, nxWord address, const void* buffer, nxWord size);

// Write a file directly in a bank.  Will return NX_NO if the contents are too big.
nxBool nxPokeBufferEx(Next N, nxByte bank, nxWord address, const void* buffer, nxWord size);

// Utility function that opens a file, reads it in, calls nxPokeBuffer, then closes the file.
nxBool nxPokeFile(Next N, nxWord address, const char* fileName);

// Read a byte from the current mapped memory.
nxByte nxPeek(Next N, nxWord address);

// Read a 16-bit value, little endian, from the current mapped memory.
nxWord nxPeek16(Next N, nxWord address);

// Read directly from a bank.
nxByte nxPeekEx(Next N, nxByte bank, nxWord p);

// Read a 16-bit value directly from a bank.
nxWord nxPeek16Ex(Next N, nxByte bank, nxWord p);

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

// PAGING
//
//          7   6   5   4   3   2   1   0
//        +---+---+---+---+---+---+---+---+
//  $7ffd |   |   |   |   |   | bits 0-2  |
//        +---+---+---+---+---+---+---+---+
//  $dffd |   |   |   |   |   | bits 3-5  |
//        +---+---+---+---+---+---+---+---+
//
#define NX_PORT_128_PAGE        0x7ffd
#define NX_PORT_NEXT_PAGE       0xdffd

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
// PNG/NIM routines
// The PNG reading routines rely on stb_image.h.  The PNG writing routines will not.
//
// This library supports a custom format called NIM files.  These are simple 2D image files that hold images already
// in Layer-2 format.  They are just a header with a stream of bytes (each byte being a pixel).  The format is:
//
//      Index   Size    Description
//      0       2       Version (0 = only version right now)
//      2       2       Width of image
//      4       2       Height of image
//      6       W*H     Image data
//----------------------------------------------------------------------------------------------------------------------

// Load a PNG file and return a 2D byte-array.  Each byte will be a pixel.  True-colour images are converted to
// a colour in the CURRENT next palette based on closest match.  You need to define NX_USE_STB when you define
// NX_IMPLEMENTATION to use this functionality.  Currently, will only respect alpha values of 0, or non-zero.  Zero
// alpha values will be translated to the transparency palette index.
nxByte* nxPngRead(Next N, const char* fileName, nxWord* width, nxWord* height);

// Free an image loaded by nxPngRead
void nxPngFree(nxByte* img);

// Write out an uncompressed PNG using the CURRENT palette.
nxBool nxPngWrite(Next N, const char* fileName, nxByte* img, int width, int height);

// Load a NIM file.  A pointer to a 2d array is returned with the width and height returned in output parameters.
nxByte* nxNimRead(const char* fileName, nxWord* width, nxWord* height);

// Free an image loaded by nxNimRead
void nxNimFree(nxByte* img);

// Save an image to a NIM file.
nxBool nxNimWrite(const char* fileName, nxByte* img, nxWord width, nxWord height);

// Save a screenshot by writing out an uncompressed PNG file.
//void nxScreenshot(Next N, const char* fileName);

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_IMPLEMENTATION

#include <assert.h>
#include <conio.h>
#include <fcntl.h>
#include <io.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define NxInternal static
#define NX_ASSERT(x, ...) assert(x)

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

NxInternal void nxMemoryFree(void* address, const char* file, int line)
{
    nxMemoryOp(address, 0, 0, file, line);
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
#define NX_FREE(address) nxMemoryFree((address), __FILE__, __LINE__)


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
// Memory Arenas
// A memory arena is an expanding buffer that can only allocate, never deallocate.  Allocation is just advancing a
// pointer, increasing the buffer if necessary
//----------------------------------------------------------------------------------------------------------------------

typedef struct
{
    nxByte* start;
    nxByte* end;
    nxInt   cursor;
}
NxArena;

#define NX_MAX(a,b) ((a) < (b) ? (b) : (a))
#define NX_ARENA_INCREMENT 4096

NxInternal void nxArenaInit(NxArena* A, nxInt initialSize)
{
    nxByte* buffer = (nxByte *)NX_ALLOC(initialSize);
    if (buffer)
    {
        A->start = buffer;
        A->end = A->start + initialSize;
        A->cursor = 0;
    }
    else
    {
        A->start = 0;
        A->end = 0;
        A->cursor = 0;
    }
}

NxInternal void nxArenaDone(NxArena* A)
{
    NX_FREE(A->start);
    A->start = A->end = 0;
    A->cursor = 0;
}

NxInternal void* nxArenaAlloc(NxArena* A, nxInt numBytes)
{
    void* p = 0;
    if ((A->start + A->cursor + numBytes) > A->end)
    {
        // We don't have enough room
        nxInt currentSize = (nxInt)(A->end - A->start);
        nxInt requiredSize = A->cursor + numBytes;
        nxInt newSize = currentSize + NX_MAX(requiredSize, NX_ARENA_INCREMENT);

        nxByte* newArena = (nxByte *)NX_REALLOC(A, currentSize, newSize);

        if (newArena)
        {
            A->start = newArena;
            A->end = newArena + newSize;
            p = nxArenaAlloc(A, numBytes);
        }
    }
    else
    {
        p = A->start + A->cursor;
        A->cursor += numBytes;
    }

    return p;
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
#define NX_NUM_PAGES        64

struct _Next
{
    Window              window;
    nxDword*            image;

    // Memory
    nxByte              banks[4];
    nxByte              pages[NX_NUM_PAGES][16384];
    nxByte              palette[256];
    nxByte              page0_2;
    nxByte              page3_5;

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
    nxByte              layer2Transparent;          // Transparent palette index
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
            nxByte bank = 5;
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
                if (pixel != N->layer2Transparent)
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
    N->page0_2 = 0;
    N->page3_5 = 0;

    for (int i = 0; i < 256; ++i) N->palette[i] = i;

    N->layer2Bank = 0;
    N->layer2BankStart = 8;
    N->layer2ShadowBankStart = 11;
    N->layer2Transparent = 0xe3;
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
        NX_FREE(N->image);
        NX_FREE(N);
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

NxInternal nxCalcMem(Next N, nxWord address, nxByte* bank, nxWord* p, nxBool isWrite)
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
    nxByte bank;
    nxWord p;
    nxCalcMem(N, address, &bank, &p, NX_YES);
    N->pages[bank][p] = b;
}

void nxPoke16(Next N, nxWord address, nxWord w)
{
    nxPoke(N, address, NX_LO(w));
    nxPoke(N, address+1, NX_HI(w));
}

void nxPokeEx(Next N, nxByte bank, nxWord address, nxByte b)
{
    address &= 0x3fff;
    N->pages[bank][address] = b;
}

void nxPoke16Ex(Next N, nxByte bank, nxWord address, nxWord w)
{
    nxPokeEx(N, bank, address, NX_LO(w));
    nxPokeEx(N, bank, address + 1, NX_HI(w));
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

nxBool nxPokeBufferEx(Next N, nxByte bank, nxWord address, const void* buffer, nxWord size)
{
    if ((nxInt)address + (nxInt)size > 16384) return NX_NO;
    nxByte* b = (nxByte *)buffer;
    for (nxWord i = 0; i < size; ++i)
    {
        nxPokeEx(N, bank, address, *b++);
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
    nxByte bank;
    nxWord p;
    nxCalcMem(N, address, &bank, &p, NX_NO);
    return nxPeekEx(N, bank, p);
}

nxWord nxPeek16(Next N, nxWord address)
{
    return nxPeek(N, address) + 256 * nxPeek(N, address + 1);
}

nxByte nxPeekEx(Next N, nxByte bank, nxWord p)
{
    p &= 0x3fff;
    return N->pages[bank][p];
}

nxWord nxPeek16Ex(Next N, nxByte bank, nxWord p)
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
    case 0xfd:
        {
            switch (h)
            {
            case 0x7f:
                N->page0_2 = (b & 0x07);
                break;

            case 0xdf:
                N->page3_5 = (b & 0x07);
                break;
            }

            // Switch slot 4
            N->banks[3] = N->page0_2 + (N->page3_5 << 3);
        }
        break;

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

            case 0x14:  // Layer 2 transparency register
                N->layer2Transparent = b;
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
// PNG and NIM support
//----------------------------------------------------------------------------------------------------------------------

#ifdef NX_USE_STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

NxInternal nxByte nxSnapPalette(Next N, nxByte r, nxByte g, nxByte b)
{
    nxByte nearestIndex;
    nxFloat nearestDistance = 1000.0;

    nxFloat rr = (nxFloat)r;
    nxFloat gg = (nxFloat)g;
    nxFloat bb = (nxFloat)b;

    for (int i = 0; i < 256; ++i)
    {
        nxFloat prr = (nxFloat)(kColour_3bit[(N->palette[i] & 0xe0) >> 5]);
        nxFloat pgg = (nxFloat)(kColour_3bit[(N->palette[i] & 0x1c) >> 2]);
        nxFloat pbb = (nxFloat)(kColour_2bit[(N->palette[i] & 0x03) >> 0]);

        nxFloat d = sqrt(rr*prr + gg*pgg + bb*pbb);
        if (d < nearestDistance)
        {
            nearestDistance = d;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

nxByte* nxPngRead(Next N, const char* filename, nxWord* width, nxWord* height)
{
    int w, h, bpp;
    nxDword* img = (nxDword *)stbi_load(filename, &w, &h, &bpp, 4);
    nxDword* in = img;
    if (!img) return 0;
    nxQword size = w * h;

    nxByte* nxtImg = NX_ALLOC(size);
    nxByte* out = nxtImg;

    // STB loads image data in format: RGBA RGBA... So *img is of the format ABGR
    for (int row = 0; row < h; ++row)
    {
        for (int col = 0; col < w; ++col)
        {
            nxByte a = (*in & 0xff000000) >> 24;
            nxByte b = (*in & 0x00ff0000) >> 16;
            nxByte g = (*in & 0x0000ff00) >> 8;
            nxByte r = (*in & 0x000000ff);

            // Search
            *out++ = a ? nxSnapPalette(N, r, g, b) : N->layer2Transparent;
            ++in;
        }
    }

    stbi_image_free(img);
    *width = w;
    *height = h;
    return nxtImg;
}

void nxPngFree(nxByte* img)
{
    NX_FREE(img);
}

#endif // NX_USE_STB


#define NX_DEFLATE_MAX_BLOCK_SIZE   65536
#define NX_BLOCK_HEADER_SIZE        5

NxInternal nxDword nxAdler32(nxDword state, const nxByte* data, nxInt len)
{
    nxWord s1 = state;
    nxWord s2 = state >> 16;
    for (nxInt i = 0; i < len; ++i)
    {
        s1 = (s1 + data[i]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    return (nxDword)s2 << 16 | s1;
}

// Table of CRCs of all 8-bit messages.
unsigned long kCrcTable[256];

// Flag: has the table been computed? Initially false.
nxBool gCrcTableComputed = NX_NO;

// Make the table for a fast CRC.
void nxCrcMakeTable(void)
{
    nxDword c;
    int n, k;

    for (n = 0; n < 256; n++) {
        c = (nxDword)n;
        for (k = 0; k < 8; k++) {
            if (c & 1)
                c = 0xedb88320L ^ (c >> 1);
            else
                c = c >> 1;
        }
        kCrcTable[n] = c;
    }
    gCrcTableComputed = NX_YES;
}

// Update a running CRC with the bytes data[0..len-1]--the CRC
// should be initialized to all 1's, and the transmitted value
// is the 1's complement of the final running CRC (see the
// nxCrc32() routine below).

nxDword nxCrc32Update(nxDword crc, void* data, nxInt len)
{
    nxDword c = crc;
    nxByte* d = (nxByte *)data;

    if (!gCrcTableComputed) nxCrcMakeTable();
    for (nxInt n = 0; n < len; n++) {
        c = kCrcTable[(c ^ d[n]) & 0xff] ^ (c >> 8);
    }
    return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
nxDword nxCrc32(void* data, nxInt len)
{
    return nxCrc32Update(0xffffffffL, data, len) ^ 0xffffffffL;
}

nxBool nxPngWrite(Next N, const char* fileName, nxByte* img, int width, int height)
{
    // Swizzle image from ARGB to ABGR
    nxDword* newImg = NX_ALLOC(sizeof(nxDword)*width*height);
    nxByte* src = (nxByte *)img;
    nxByte* dst = (nxByte *)newImg;

    // Data is BGRABGRA...  should be RGBARGBA...
    for (int yy = 0; yy < height; ++yy)
    {
        for (int xx = 0; xx < width; ++xx)
        {
            // Write red
            dst[0] = kColour_3bit[(*src & 0xe0) >> 5];
            // Write green
            dst[1] = kColour_3bit[(*src & 0x1c) >> 2];
            // Write blue
            dst[2] = kColour_2bit[(*src & 0x03)];
            // Write alpha
            dst[3] = *src == N->layer2Transparent ? 0x00 : 0xff;
            ++src;
            dst += 4;
        }
    }

    // Calculate size of PNG
    nxInt fileSize = 0;
    nxInt lineSize = width * sizeof(nxDword) + 1;
    nxInt imgSize = lineSize * height;
    nxInt overheadSize = imgSize / NX_DEFLATE_MAX_BLOCK_SIZE;
    if (overheadSize * NX_DEFLATE_MAX_BLOCK_SIZE < imgSize)
    {
        ++overheadSize;
    }
    overheadSize = overheadSize * 5 + 6;
    nxInt dataSize = imgSize + overheadSize;      // size of zlib + deflate output
    nxDword adler = 1;
    nxInt deflateRemain = imgSize;
    nxByte* imgBytes = (nxByte *)newImg;

    fileSize = 43;
    fileSize += dataSize + 4;       // IDAT deflated data

                                    // Open arena
    NxArena m;
    nxArenaInit(&m, dataSize + 1024);
    nxByte* p = nxArenaAlloc(&m, 43);
    nxByte* start = p;

    // Write file format
    nxByte header[] = {
        // PNG header
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
        // IHDR chunk
        0x00, 0x00, 0x00, 0x0d,                                 // length
        0x49, 0x48, 0x44, 0x52,                                 // 'IHDR'
        width >> 24, width >> 16, width >> 8, width,            // width
        height >> 24, height >> 16, height >> 8, height,        // height
        0x08, 0x06, 0x00, 0x00, 0x00,                           // 8-bit depth, true-colour+alpha format
        0x00, 0x00, 0x00, 0x00,                                 // CRC-32 checksum
        // IDAT chunk
        (nxByte)(dataSize >> 24), (nxByte)(dataSize >> 16), (nxByte)(dataSize >> 8), (nxByte)dataSize,
        0x49, 0x44, 0x41, 0x54,                                 // 'IDAT'
        // Deflate data
        0x08, 0x1d,                                             // ZLib CMF, Flags (Compression level 0)
    };
    nxMemoryCopy(header, p, sizeof(header) / sizeof(header[0]));
    nxDword crc = nxCrc32(&p[12], 17);
    p[29] = crc >> 24;
    p[30] = crc >> 16;
    p[31] = crc >> 8;
    p[32] = crc;
    crc = nxCrc32(&p[37], 6);

    // Write out the pixel data compressed
    int x = 0;
    int y = 0;
    nxInt count = width * height * sizeof(nxDword);
    nxInt deflateFilled = 0;
    while (count > 0)
    {
        // Start DEFALTE block
        if (deflateFilled == 0)
        {
            nxDword size = NX_DEFLATE_MAX_BLOCK_SIZE;
            if (deflateRemain < (nxInt)size)
            {
                size = (nxWord)deflateRemain;
            }
            nxByte blockHeader[NX_BLOCK_HEADER_SIZE] = {
                deflateRemain <= NX_DEFLATE_MAX_BLOCK_SIZE ? 1 : 0,
                size,
                size >> 8,
                (size) ^ 0xff,
                (size >> 8) ^ 0xff
            };
            p = nxArenaAlloc(&m, sizeof(blockHeader));
            nxMemoryCopy(blockHeader, p, sizeof(blockHeader));
            crc = nxCrc32Update(crc, blockHeader, sizeof(blockHeader));
        }

        // Calculate number of bytes to write in this loop iteration
        nxDword n = 0xffffffff;
        if ((nxDword)count < n)
        {
            n = (nxDword)count;
        }
        if ((nxDword)(lineSize - x) < n)
        {
            n = (nxDword)(lineSize - x);
        }
        NX_ASSERT(deflateFilled < NX_DEFLATE_MAX_BLOCK_SIZE);
        if ((nxDword)(NX_DEFLATE_MAX_BLOCK_SIZE - deflateFilled) < n)
        {
            n = (nxDword)(NX_DEFLATE_MAX_BLOCK_SIZE - deflateFilled);
        }
        NX_ASSERT(n != 0);

        // Beginning of row - write filter method
        if (x == 0)
        {
            p = nxArenaAlloc(&m, 1);
            *p = 0;
            crc = nxCrc32Update(crc, p, 1);
            adler = nxAdler32(adler, p, 1);
            --deflateRemain;
            ++deflateFilled;
            ++x;
            if (count != n) --n;
        }

        // Write bytes and update checksums
        p = nxArenaAlloc(&m, n);
        nxMemoryCopy(imgBytes, p, n);
        crc = nxCrc32Update(crc, imgBytes, n);
        adler = nxAdler32(adler, imgBytes, n);
        imgBytes += n;
        count -= n;
        deflateRemain -= n;
        deflateFilled += n;
        if (deflateFilled == NX_DEFLATE_MAX_BLOCK_SIZE)
        {
            deflateFilled = 0;
        }
        x += n;
        if (x == lineSize) {
            x = 0;
            ++y;
            if (y == height)
            {
                // Wrap things up
                nxByte footer[20] = {
                    adler >> 24, adler >> 16, adler >> 8, adler,    // Adler checksum
                    0, 0, 0, 0,                                     // Chunk crc-32 checksum
                                                                    // IEND chunk
                                                                    0x00, 0x00, 0x00, 0x00,
                                                                    0x49, 0x45, 0x4e, 0x44,
                                                                    0xae, 0x42, 0x60, 0x82,
                };
                crc = nxCrc32Update(crc, footer, 20);
                footer[4] = crc >> 24;
                footer[5] = crc >> 16;
                footer[6] = crc >> 8;
                footer[7] = crc;

                p = nxArenaAlloc(&m, 20);
                nxMemoryCopy(footer, p, 20);
                break;
            }
        }
    }

    // Transfer file
    NX_FREE(newImg);
    nxByte* end = nxArenaAlloc(&m, 0);
    nxInt numBytes = (nxInt)(end - start);
    NxData d = nxDataMake(fileName, numBytes);
    if (d.bytes)
    {
        nxMemoryCopy(start, d.bytes, numBytes);
        nxDataUnload(d);
        nxArenaDone(&m);
        return NX_YES;
    }
    else
    {
        return NX_NO;
    }
}

nxByte* nxNimRead(const char* fileName, nxWord* width, nxWord* height)
{
    NxData d = nxDataLoad(fileName);
    if (d.bytes)
    {
        nxWord* header = (nxWord *)d.bytes;
        if (header[0] != 0) return 0;

        *width = header[1];
        *height = header[2];
        nxInt size = (nxInt)header[1] * (nxInt)header[2];
        nxByte* img = NX_ALLOC(size);
        if (img)
        {
            nxMemoryCopy(&d.bytes[6], img, size);
        }
        else
        {
            return 0;
        }

        nxDataUnload(d);
        return img;
    }

    return 0;
}

void nxNimFree(nxByte* img)
{
    NX_FREE(img);
}

nxBool nxNimWrite(const char* fileName, nxByte* img, nxWord width, nxWord height)
{
    nxInt imgSize = (nxInt)width * (nxInt)height;
    nxWord* header;

    NxData d = nxDataMake(fileName, imgSize + 3 * sizeof(nxWord));
    if (d.bytes)
    {
        header = (nxWord *)d.bytes;
        header[0] = 0x0000;
        header[1] = width;
        header[2] = height;
        nxMemoryCopy(img, &header[3], imgSize);

        nxDataUnload(d);
        return NX_YES;
    }

    return NX_NO;
}

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

#endif // NX_IMPLEMENTATION
