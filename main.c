#include <unistd.h>
#include <fcntl.h>
#include "SDL.h"

int width = 1920;
int height = 1080;

SDL_TimerID timer1;
SDL_Renderer* renderer;
unsigned int inbuf_len = 0;
unsigned char inbuf[4096];

static Uint32 event_push(Uint32 interval, void* param);
void frame_render();

int main(int argc, char* argv[]) {
    int fps = 60;
    int delay = 1000/fps;
    int done = 0;

    if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
		fprintf(stderr, "Couldn't initialize SDL: %s\n",
			SDL_GetError());
		exit(1);
	}

    SDL_Window* window = SDL_CreateWindow("entropy_viewer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
    if (window == NULL) {
        printf("Error creating window\n");
        exit(1);
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (renderer == NULL) {
        printf("Error creating renderer\n");
        exit(1);
    }

    int flags = fcntl(0, F_GETFL, 0);
    if (flags == -1) exit(1);
    flags = flags | O_NONBLOCK;
    if (fcntl(0, F_SETFL, flags) != 0) exit(1);


    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    timer1 = SDL_AddTimer(delay, event_push, NULL);

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

// average
unsigned char r() {
    int res = 0;
    for (int i=0; i<4096; i++) {
        res = res + (int) inbuf[i];
    }
    return (unsigned char)(res / 4096);
}

// xor
unsigned char g() {
    unsigned char res = 0;
    for (int i=0; i<4096; i++) {
        res = res ^ inbuf[i];
    }
    return res;
}

// proportion > 0
unsigned char b() {
    int res = 0;
    for (int i=0; i<4096; i++) {
        if (inbuf[i] > 0) res++;
    }
    return (unsigned char) (res / (4096/256));
}

void frame_render() {
    int pixels = 0;
    while (1) {
        int char_read = read(0, inbuf + inbuf_len, 4096 - inbuf_len);
        if (char_read < 0) {
            break; // nothing to read
        }
        inbuf_len += char_read;

        if (inbuf_len >= 4096) { // we have a full buffer, draw a pixel
            unsigned char red = r();
            unsigned char green = g();
            unsigned char blue = b();

            SDL_SetRenderDrawColor(renderer, r(), g(), b(), 255);
            SDL_RenderDrawPoint(renderer, xpos, ypos);

            pixels++;
            
            xpos++;
            if (xpos >= width) {
                xpos = 0;
                ypos++;
            }
            if (ypos >= height) {
                ypos = 0;
            }
            inbuf_len = 0;
        }
        if (pixels > width) break;
    }
    SDL_RenderPresent(renderer);
}

static Uint32 event_push(Uint32 interval, void* param)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	SDL_PushEvent(&event);
	return 0;
}