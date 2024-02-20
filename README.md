# entropy_viewer

Displays entropy read from stdin in 4K blocks, one pixel at a time, using SDL2. 

Kind-of like [playfile](https://github.com/gm-stack/playfile) (which I wrote 14 years ago) - but looking at a 4K block at a time, not one pixel at a time.

Each pixel's colour is:

- red: average value of byte
- green: all bytes xor'd together
- blue: proportion of pixels in 4K block that are not 0

Written to see if a hard drive being recovered was still returning all zeros.

Supports drawing in normal mode (draw from left to right, top to bottom, then start again at top left), and waterfall mode (draw a line, then shift screen down).

## Screenshot

![screenshot showing static like pattern at top of image](screenshot.jpg)

## Compilation

Requires `sdl2` and `pkg-config` installed.

```bash
clang main.c -o main `pkg-config sdl2 --cflags --libs`
```

## Usage

```bash
cat somefile.img | ./main -w 1920 -h 1080
```

```bash
tail -f somefile.img | ./main -w 640 -h 480 -a
```

## Todo

- Colour scheme could be configurable