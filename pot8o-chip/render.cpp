#include <chrono>
#include <SDL.h>
#include "render.h"

Render::Render() {
    window =
        SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 256, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(renderer, 8, 8);
    texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void Render::drawGraphics(std::array<unsigned short, 64 * 32>& frame) {
    SDL_UpdateTexture(texture, NULL, &frame, 128);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}
