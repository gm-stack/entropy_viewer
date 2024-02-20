# entropy_viewer

Displays entropy read from stdin in 4K blocks, one pixel at a time, using SDL2.

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