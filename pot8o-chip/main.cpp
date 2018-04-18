#include "chip8.h"
#include <SDL.h>
#include <iostream>

Chip8 chip8;

int main(int argc, char * argv[])
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		std::cout << "SDL initialization failed. SDL Error: " << SDL_GetError();
	}
	else
	{
		std::cout << "SDL initialization succeeded!";
	}

	// Initialize the Chip8 system and load the game into the memory  
	chip8.initialize();
	chip8.loadGame("pong");

	// Emulation loop
	for (;;)
	{
		// Emulate one cycle
		chip8.emulateCycle();

		// If the draw flag is set, update the screen
		if (chip8.drawFlag)
			drawGraphics();

		// Store key press state (Press and Release)
		chip8.setKeys();
	}

	std::cin.get();
	return 0;
}
