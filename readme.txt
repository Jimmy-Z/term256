- Written for DS but had GBA in mind when I wrote it, should not be hard to port.
- Uses 8bpp bmp bg mode because tile mode has fixed width of 8, if that's OK for you, devKitPro already has one.
- Supported font width: 6, yes, only 6, the test code uses the 6x10 font from Linux kernel.
- With font width 6, we can fit 42 characters on a single line on DS screen, compared to 32 for DKP console, that's the main reason this was written.
- 256 ANSI colors, each can be used on forground and/or background, like a real 256 color terminal, this is the second reason this was written.
- Support some ANSI escape codes.
- LUT optimized font reading, resonably fast.
- Partial hardware scroll, that's really fast, I suppose this feature won't work on GBA.

Cons:
- Magnitudes slower than DKP console and also cosumes more VRAM(64KB vs 4KB) and possiblely a little more RAM too due to LUT(6*64 bytes), but I suppose it's still reasonable, a test case which wrote more than two screens of text and extensive ANSI escape code color manipulating costed a little more than 50ms, on a DSi.
- Not thoroughly tested.

Ideas and thoughts:
- It could be possible to use 2x bg to eliminate software scroll completely, and sounds like fun to code, but I don't think I could make the code compatible with GBA then.
- I guess DKP console used 1 tile for each character, then 16 color palattes gave us 16 forground color choices, but background color is stuck at black, I suppose we can use a custom palatte to have 16 forground/background combinations, not necessarily 4 forground colors * 4 background colors, like white on white won't make much sense, just choose what you'd use, 16 custom combinations is not that bad, especially comparing with 16 colors on black(including black on black).
- Or, take another way, use 15 tiles per character, each utilizing a different color slot in the palatte for background, and we can still switch color palattes for forground color, this way we could have 16*15 forground*background choices, it will cosumes 15 times VRAM for tiles, but retains the speed advantage. But still stuck with 8x8 font.
- Is it possible to use 3x overlapping tile bg to support 6x8 font? well if there is any good looking 6x8 font.
- Yeah I'm kinda obsessed.

