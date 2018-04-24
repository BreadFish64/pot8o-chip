#include <algorithm>
#include <iostream>
#include <SDL.h>
#include "chip8.h"
#include "main.h"

Chip8 chip8;

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window* window =
        SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 256, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(renderer, 8, 8);
    SDL_Texture* texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);

    chip8.initialize();
    chip8.loadGame(argv[0]);

    // Emulation loop
    for (;;) {
        chip8.emulateCycle();

        if (chip8.vf) {
            drawGraphics(renderer, texture, chip8);
            chip8.vf = false;
        };

        // Store key press state (Press and Release)
        // chip8.setKeys();
    }

    std::cin.get();
    return 0;
}

void drawGraphics(SDL_Renderer* renderer, SDL_Texture* texture, Chip8 chip8) {
    std::array<int, 64 * 32> pixels;
    for (int y = 0; y < chip8.gfx.size(); y++) {
        for (int x = 0; x < chip8.gfx[y].size(); x++) {
            printf("%d", chip8.gfx[y][x]);
            int offset = y * 64 + x;
            pixels[offset] = chip8.gfx[y][x] ? 0x00000000 : 0xFFFFFFFF;
        }
        printf("\n");
    }
    printf("\n");
    SDL_UpdateTexture(texture, NULL, &pixels, 256);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    chip8.vf = false;
}
