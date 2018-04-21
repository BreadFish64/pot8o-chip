#include "main.h"
#include "chip8.h"
#include <SDL.h>
#include <algorithm>
#include <iostream>

Chip8 chip8;

int main(int argc, char * argv[]) {
  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_Window *window = SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED,
                                        SDL_WINDOWPOS_UNDEFINED, 64, 32, 0);
  SDL_Renderer *renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture *texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                        SDL_TEXTUREACCESS_STREAMING, 64, 32);

  chip8.initialize();
  chip8.loadGame(argv[0]);

  // Emulation loop
  for (;;) {
    chip8.emulateCycle();

    if (chip8.vf)
    	drawGraphics(renderer, texture, chip8);

    // Store key press state (Press and Release)
    // chip8.setKeys();
	}

	std::cin.get();
	return 0;
}

void drawGraphics(SDL_Renderer* renderer, SDL_Texture* texture, Chip8 chip8) {
	std::array<unsigned char, 64 * 32 * 4> pixels;
    for(int y = 0 ; y < chip8.gfx.size() ; y++) {
		for (int x = 0; x < chip8.gfx[y].size(); x++) {
			if (y == 63 && x == 31) {
				printf("meep");
			}
			int offset = y * 64 + x * 4;
			std::fill(pixels.begin() + offset, pixels.begin() + offset + 4, chip8.gfx[y][x] ? 0x00 : 0xFF);
		}
    }
	SDL_UpdateTexture(texture, NULL, &pixels, 64 * 4);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
	chip8.vf = false;
}
