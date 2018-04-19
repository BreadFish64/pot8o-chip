#include "chip8.h"
#include <SDL.h>
#include <iostream>

Chip8 chip8;

int main(int argc, char * argv[]) {
	if (SDL_Init(SDL_INIT_VIDEO) != 0) return 1;

	SDL_Window* window = SDL_CreateWindow("Video", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);
	if (!window) return 1;

	SDL_Surface* surface = SDL_GetWindowSurface(window);

	SDL_Renderer* renderer = SDL_CreateSoftwareRenderer(surface);
	if (!renderer) return 1;

	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
	SDL_RenderClear(renderer);

	std::string path;
	std::cin >> path;

	chip8.initialize();
	chip8.loadGame(path);

	// Emulation loop
	for (;;)
	{
		chip8.emulateCycle();

		// If the draw flag is set, update the screen
		//if (chip8.drawFlag)
		//	drawGraphics();

		// Store key press state (Press and Release)
		//chip8.setKeys();
	}

	std::cin.get();
	return 0;
}

void drawGraphics(SDL_Surface* surface, Chip8 chip8) {
    for(int x = 0 ; x < chip8.gfx.size() ; x++) {
		for (int y = 0; y < chip8.gfx[x].size(); y++) {
			unsigned char *pixels = static_cast<unsigned char*>(surface->pixels);
			pixels[(y * surface->w) + x] = chip8.gfx[x][y];
		}
    }
}
