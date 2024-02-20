# entropy_viewer

Displays entropy read from stdin in 4K blocks, one pixel at a time, using SDL2. 

Kind-of like [playfile](https://github.com/gm-stack/playfile) (which I wrote 14 years ago) - but looking at a 4K block at a time, not one pixel at a time.

Each pixel's colour is:

- red: average value of byte
- green: all bytes xor'd together
- blue: proportion of pixels in 4K block that are not 0

Written to see if a hard drive being recovered was still returning all zeros.

## Screenshot

![screenshot showing static like pattern at top of image](screenshot.jpg)

## Compilation

Requires `sdl2` and `pkg-config` installed.

```bash
clang main.c -o main `pkg-config sdl2 --cflags --libs`
```

## Usage

```bash
cat somefile.img | ./main
```

```bash
tail -f somefile.img | ./main
```

## Todo

- Speed can probably be improved by writing to a texture directly rather than using `SDL_SetRenderDrawColor()`.
- Program could take arguments for width/height/fps/file to open
- Colour scheme could be configurable