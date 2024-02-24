# entropy_viewer

Displays entropy read from stdin for a number of blocks of data, using SDL2. 

By default, every 4096 bytes is one pixel. The number of bytes and the scaling factor in pixels can be configured.

Kind-of like [playfile](https://github.com/gm-stack/playfile) (which I wrote 14 years ago) - but looking at a block at a time, not one pixel at a time - effectively "zoomed out".

Each pixel's colour is:

- red: average value of byte across block
- green: all bytes in block xor'd together
- blue: proportion of pixels in the block that are not 0

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

```
Usage: cat <file> | ./main -w width -h height [-f fps] [-a] [-b size] [-x] [-s scale]
               -w : window width in pixels
               -h : window height in pixels
               -f : FPS (default 60)
               -a : draw in waterfall mode - draw a full line then shift down
                    (default: draw \"CRT style\" - left to right, top to bottom,
                    then start again at top left)
               -b : buffer size per block in bytes (default 4096)
               -x : exit when EOF reached
               -s : scaling factor: each block is n pixels
```

```bash
cat somefile.img | ./main -w 1920 -h 1080
```

```bash
tail -f somefile.img | ./main -w 640 -h 480 -a -b 16384
```

## Todo

- Colour scheme could be more configurable