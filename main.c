#include <unistd.h>
#include <fcntl.h>
#include "SDL.h"

int width = 0;
int height = 0;
int waterfall = 0;

SDL_TimerID timer1;
SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* screen_texture;
Uint32 *pixels;

unsigned int inbuf_len = 0;
unsigned char inbuf[4096];

static Uint32 event_push(Uint32 interval, void* param);
void frame_render();

int main(int argc, char* argv[]) {
    int fps = 60;
    int delay = 1000/fps;
    int done = 0;

    int c;
	while ((c = getopt(argc, argv, "aw:h:f:")) != -1) {
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
			case 'a':
				waterfall = 1;
				break;
		}
	}

    if (width == 0 || height == 0) {
		printf("Usage: cat <file> | %s -w width -h height [-f fps] [-a] file\n"
			   "-w : window width in pixels\n"
			   "-h : window height in pixels\n"
			   "-f : FPS (default 60)\n"
			   "-a : draw in waterfall mode\n"
			   , argv[0]);
		exit(1);
	}

    // setup SDL
    if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}

    window = SDL_CreateWindow("entropy_viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    if (window == NULL) {
        printf("Error creating window\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL) {
        printf("Error creating renderer\n");
        exit(1);
    }

    // create screen texture and pixel buffer
    screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    pixels = malloc(width * height * 4);

    // set stdin to non blocking
    int flags = fcntl(0, F_GETFL, 0);
    if (flags == -1) exit(1);
    flags = flags | O_NONBLOCK;
    if (fcntl(0, F_SETFL, flags) != 0) exit(1);

    // draw the screen white
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
					timer1 = SDL_AddTimer(delay, event_push, NULL);
                    frame_render();
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

int xpos = 0;
int ypos = 0;

// average of all
unsigned char r() {
    int res = 0;
    for (int i=0; i<4096; i++) {
        res = res + (int) inbuf[i];
    }
    return (unsigned char)(res / 4096);
}

// xor of all
unsigned char g() {
    unsigned char res = 0;
    for (int i=0; i<4096; i++) {
        res = res ^ inbuf[i];
    }
    return res;
}

// proportion of values > 0 in all
unsigned char b() {
    int res = 0;
    for (int i=0; i<4096; i++) {
        if (inbuf[i] > 0) res++;
    }
    return (unsigned char) (res / (4096/256));
}

void frame_render() {
    int num_pixels = 0; // number of pixels drawn
    while (1) {
        // attempt to read some characters, up to what's left in the buffer
        int char_read = read(0, inbuf + inbuf_len, 4096 - inbuf_len);
        if (char_read < 0) {
            break; // nothing more to read, just draw what we have and check again later
        }
        inbuf_len += char_read;

        if (inbuf_len >= 4096) { // we have a full buffer, draw a pixel
            inbuf_len = 0; // reset to start of buffer

            // calculate colours
            unsigned char red = r();
            unsigned char green = g();
            unsigned char blue = b();

            // tweak pixel in pixels array
            pixels[xpos + (ypos * width)] = 0xFF000000 | (red << 16) | (green << 8) | blue;
            num_pixels++; // count pixels we've drawn
            
            // increment X position
            xpos++;
            if (waterfall) {
                if (xpos >= width) { // once we have completed a line
                    memmove(&pixels[width], pixels, width * (height - 1) * 4); // move the screen down a line
                    xpos = 0; // reset position to X
                    // y position always stays at top
                }
            } else {
                if (xpos >= width) { // once completed a line
                    xpos = 0; // reset to left
                    ypos++; // move down a line
                }
                if (ypos >= height) { // once at bottom of screen
                    ypos = 0; // go back up to top
                }
            }
        }
        if (num_pixels > width) break; // if we've drawn enough for a whole line, break and draw it already
                                       // (otherwise we can sit here forever reading if data 
                                       // is coming in quicker than we can draw)
    }
    // clear screen, update texture, paint texture to screen buffer, draw screen buffer
    SDL_RenderClear(renderer);
    SDL_UpdateTexture(screen_texture, NULL, pixels, width*4);
    SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

static Uint32 event_push(Uint32 interval, void* param)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
	return 0;
}