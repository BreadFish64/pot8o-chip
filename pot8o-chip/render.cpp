#include <chrono>
#include <SDL.h>
#include "render.h"

Render::Render(std::queue<std::array<std::array<bool, 64>, 32>>& buffer,
               std::timed_mutex& buffer_mutex)
    : buffer(buffer), buffer_mutex(buffer_mutex) {
    window =
        SDL_CreateWindow("Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 256, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetScale(renderer, 8, 8);
    texture =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 64, 32);
}

void Render::drawGraphics(std::array<std::array<bool, 64>, 32> frame) {
    std::array<int, 64 * 32> pixels;
    for (int y = 0; y < frame.size(); y++) {
        for (int x = 0; x < frame[y].size(); x++) {
            printf("%d", frame[y][x]);
            int offset = y * 64 + x;
            pixels[offset] = frame[y][x] ? 0x00000000 : 0xFFFFFFFF;
        }
        printf("\n");
    }
    printf("\n");
    SDL_UpdateTexture(texture, NULL, &pixels, 256);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void Render::run() {
    while (true) {
        if (!buffer.empty()) {
            std::array<std::array<bool, 64>, 32> frame = buffer.front();
            buffer.pop();
            drawGraphics(frame);
        }
    }
}
