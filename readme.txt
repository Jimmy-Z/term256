Written for DS but had GBA in mind when I wrote it, should not be hard to port back.

- Uses 8bpp bmp bg mode because tile mode has fixed width of 8, if that's OK for you, devKitPro already has one.
- Supported font width: 6, yes, only 6.
- LUT optimized font reading, resonably fast.
- Hardware scrolling, that's really fast, I suppose this feature won't work on GBA.
- 256 ANSI colors.
- But ANSI escape codes not supported, yet.

- Is it possible to use 3x overlapping bg to support 6*8 font? well if there is any good looking 6*8 font.

