#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "SDL.h"

int width = 0;
int height = 0;
int pixel_width = 0;
int pixel_height = 0;
unsigned int scale = 1;

int waterfall = 0;
int done = 0;
int eof = 0;
int exit_eof = 0;

int xpos = 0;
int ypos = 0;

SDL_TimerID timer1;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* screen_texture;
Uint32 *pixels;

unsigned int inbuf_len = 0;
unsigned int inbuf_size = 4096;
unsigned char *inbuf;

static Uint32 event_push(Uint32 interval, void* param);
void frame_render();
void repaint();

int main(int argc, char* argv[]) {
    int fps = 60;
    int delay = 1000/fps;

    int c;
	while ((c = getopt(argc, argv, "axw:h:f:b:s:")) != -1) {
		switch (c) {
			case 'w':
				width = atoi(optarg);
				break;
			case 'h':
				height = atoi(optarg);
				break;
			case 'f':
				fps = atoi(optarg);
				delay = 1000/fps;
				break;
            case 'b':
                inbuf_size = strtoul(optarg, NULL, 0);
                break;
			case 'a':
				waterfall = 1;
				break;
            case 'x':
                exit_eof = 1;
                break;
            case 's':
                scale = strtoul(optarg, NULL, 0);
                break;
		}
	}

    if (width == 0 || height == 0) {
		printf("Usage: cat <file> | %s -w width -h height [-f fps] [-a] [-b size] [-x] [-s scale]\n"
			   "-w : window width in pixels\n"
			   "-h : window height in pixels\n"
			   "-f : FPS (default 60)\n"
			   "-a : draw in waterfall mode - draw a full line then shift down\n"
               "     (default: draw \"CRT style\" - left to right, top to bottom,\n"
               "     then start again at top left)\n"
               "-b : buffer size per block in bytes (default 4096)\n"
               "-x : exit when EOF reached\n"
               "-s : scaling factor: each block is n pixels\n"
			   , argv[0]);
		exit(1);
	}

    if (inbuf_size == 0) {
        printf("invalid inbuf_size %i - must be >0\n", inbuf_size);
        exit(1);
    }

    if (scale == 0) {
        printf("invalid scale %u - must be >0\n", scale);
        exit(1);
    }

    pixel_width = width / scale;
    pixel_height = height / scale;

    inbuf = malloc(inbuf_size);

    // setup SDL
    if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}

    window = SDL_CreateWindow("entropy_viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        printf("Error creating window\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Error creating renderer\n");
        exit(1);
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0); // set scale to nearest neighbour

    // create screen texture and pixel buffer, clear to 50% grey
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width/scale, height/scale);
    unsigned int pixels_size = pixel_width * pixel_height * 4;
    pixels = malloc(pixels_size);
    memset(pixels, 0x7F, pixels_size);

    // set stdin to non blocking
    int flags = fcntl(0, F_GETFL, 0);
    if (flags == -1) exit(1);
    flags = flags | O_NONBLOCK;
    if (fcntl(0, F_SETFL, flags) != 0) exit(1);

    // draw the screen white before a frame based on input data has been drawn
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    // set a timer to draw frame
    timer1 = SDL_AddTimer(delay, event_push, NULL);

    // event loop
    done = 0;
	while ( !done ) {
        SDL_Event event;
		while ( SDL_PollEvent(&event) ) {
			switch (event.type) {
				case SDL_MOUSEMOTION:
					break;
				case SDL_MOUSEBUTTONDOWN:
					break;
				case SDL_KEYDOWN:
					break;
				case SDL_QUIT:
					printf("Exiting\n");
					SDL_Quit();
					return 0;
				case SDL_USEREVENT:
					if (eof == 1) break; 
                    timer1 = SDL_AddTimer(delay, event_push, NULL);
                    frame_render();
					break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_RESIZED) {                        
                        // calculate old pixel array size, set new width and height, calculate new size
                        unsigned int old_len = pixel_width * pixel_height * 4;
                        width = event.window.data1;
                        height = event.window.data2;
                        pixel_width = width / scale;
                        pixel_height = height / scale;
                        unsigned int new_len = pixel_width * pixel_height * 4;
                        
                        // allocate new pixels array, clear to grey and copy old pixels into it
                        Uint32 *new_pixels = malloc(new_len);
                        memset(new_pixels, 0x7F, new_len);
                        memcpy(new_pixels, pixels, (new_len < old_len) ? new_len : old_len);
                        free(pixels);
                        pixels = new_pixels;
                        
                        // create new screen texture
                        SDL_DestroyTexture(screen_texture);
                        screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, pixel_width, pixel_height);

                        // update new screen texture with new pixels, render
                        repaint();
                        
                        // ensure next pixel isn't 
                        if (xpos >= pixel_width) xpos = pixel_width;
                        if (ypos >= pixel_height) ypos = 0;
                    }
                    break;
				default:
					break;
			}
		}
	}
	
	/* Clean up the SDL library */
	SDL_Quit();
	return(0);
}

// average of all
// fixme: this can overflow
unsigned char r() {
    unsigned int res = 0;
    for (int i=0; i<inbuf_size; i++) {
        res = res + (int) inbuf[i];
    }
    return (unsigned char)(res / inbuf_size);
}

// xor of all
unsigned char g() {
    unsigned char res = 0;
    for (int i=0; i<inbuf_size; i++) {
        res = res ^ inbuf[i];
    }
    return res;
}

// proportion of values > 0 in all
// fixme: set limit for 
unsigned char b() {
    unsigned int res = 0;
    for (int i=0; i<inbuf_size; i++) {
        if (inbuf[i] > 0) res++;
    }
    return (unsigned char) ((float)res / ((float)inbuf_size/(float)256));
}

void repaint() {
    // clear screen, update texture, paint texture to screen buffer, draw screen buffer
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(screen_texture, NULL, pixels, pixel_width*4);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void frame_render() {
    int num_pixels = 0; // number of pixels drawn
    
    while (1) {
        // attempt to read some characters, up to what's left in the buffer
        int char_read = read(0, inbuf + inbuf_len, inbuf_size - inbuf_len);
        if (char_read == 0) { // if we hit end of file
            eof = 1;
            if (exit_eof) done = 1;
            printf("End of file reached.\n");
            break;
        }
        if (char_read < 0) { // if an error condition occurred
            if (errno == EAGAIN) { // not a real error - just no more data to read
                break; // nothing more to read, just draw what we have and check again later
            } else {            
                done = 1; // other error condition = exit with error printed
                printf("read error %s, exiting\n", strerror(errno));
                return;
            }
        }
        inbuf_len += char_read;

        if (inbuf_len >= inbuf_size) { // we have a full buffer, draw a pixel
            inbuf_len = 0; // reset to start of buffer

            // calculate colours
            unsigned char red = r();
            unsigned char green = g();
            unsigned char blue = b();

            // tweak pixel in pixels array
            pixels[xpos + (ypos * pixel_width)] = 0xFF000000 | (red << 16) | (green << 8) | blue;
            num_pixels++; // count pixels we've drawn
            
            // increment X position
            xpos++;
            if (waterfall) {
                if (xpos >= pixel_width) { // once we have completed a line
                    repaint();
                    memmove(&pixels[pixel_width], pixels, pixel_width * (pixel_height - 1) * 4); // move the screen down a line
                    xpos = 0; // reset position to X
                    // y position always stays at top
                }
            } else {
                if (xpos >= pixel_width) { // once completed a line
                    xpos = 0; // reset to left
                    ypos++; // move down a line
                }
                if (ypos >= pixel_height) { // once at bottom of screen
                    ypos = 0; // go back up to top
                }
            }
        }
        if (num_pixels > pixel_width) break; // if we've drawn enough for a whole line, break and draw it already
                                             // then go handle other events.
                                             // (otherwise we can sit here forever reading if data 
                                             // is coming in quicker than we can draw)
    }

    if (num_pixels == 0) return; // if no pixels were updated, just return
    if (!waterfall) repaint(); // draw if not waterfall mode - waterfall mode draws only on completed line
}

static Uint32 event_push(Uint32 interval, void* param)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
	return 0;
}