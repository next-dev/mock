# ZX Spectrum Next Mock library

This is a header-only library for mocking Next hardware for quick PC-based prototyping.  This will create a single
window that will emulate the video modes of the Next, and the various input devices (including keyboard).  Eventually,
it will emulate sound devices.

This library is ongoing work as I prototype my own Next projects.  Feel free to use it for your own prototypes.  I
welcome pull requests if you cannot wait for me to add features.

It works by allowing an API to access memory and ports to peek, poke, in and out like you normally do on the real
hardware.  There are a few convenience functions but I prefer my API to be small and close to the "hardware" as
possible.

## Features currently implemented

- 4 zoom modes (accessible to function keys 1-4).
- Original 48K ULA (including border).
- 512K extra memory (40 pages).
- Layer 2, including the transparency, paging control port and bank start registers.
- RAM only paging using ports $7FFD and $DFFD.
- PNG and NIM graphics file loading and saving.

## Features not implemented but planned for the future

- Screenshot support.
- Keyboard input.
- Debug mode (switches border to unique colour and enables debug keyboard commands).
- Kempston mouse and joystick (via XInput devices).
- 128K Spectrum ROM paging support.
- Full Next video support (including ULAnext, Timex modes, sprites & priorities).
- AY3-8912 support.

# How to use the library

Just include it.  That's all.  However, only one .c/.cpp file MUST define NX_IMPLEMENTATION so that the implementation
is pulled into one object file.  The typical code to use it looks like this:

```
Next N = nxOpen();

// Initialise memory and ports here using the memory/port APIs.

while (nxUpdate(N, &MyFrameFunc))
{
    ... Run your code ...
}

nxClose(N);
```

The function `nxUpdate` allows a custom function to be called once every 1/50th second after which time the VRAM is
re-rendered.  All frame sync code should be in this custom function.  Any code you want to run that doesn't need to
be synchronised with the frame should be inside the while loop.

Please read the top of the header file "next.h" for more information.

# NIM file format

This library introduces a simple file format for Next graphics.  It uses a 6 byte header followed by the pixel data,
which is one byte per pixel.  Each pixel byte indexes the current palette.  The header format is:

| Index | Size | Description         |
|-------|------|---------------------|
| 0     | 2    | File format version |
| 2     | 2    | Width               |
| 4     | 2    | Height              |

The file format version currently is always 0.

# Comments, questions, suggestions...

Please send comment suggestions to mattie.davies@gmail.com and I will get back to you.  If the same questions keep
cropping up I might start a FAQ.
