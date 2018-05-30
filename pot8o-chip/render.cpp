#include <chrono>
#include <SDL.h>
#include "render.h"

Render::Render() {
    window =
        SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 256, 0);
    // SDL_SetWindowResizable(window, SDL_TRUE);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(renderer, 8, 8);
    texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void Render::drawGraphics(std::array<bool, 64 * 32> frame) {
    std::array<int, 64 * 32> pixels;
    for (int i = 0; i < frame.size(); i++) {
        pixels[i] = frame[i] ? 0x00000000 : 0xFFFFFFFF;
    }

    SDL_UpdateTexture(texture, NULL, &pixels, 256);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
