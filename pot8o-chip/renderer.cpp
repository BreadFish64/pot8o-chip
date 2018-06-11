#include <SDL.h>
#include "renderer.h"

Renderer::Renderer() {
    window = std::unique_ptr<SDL_Window, SDL_Deleter>(
        SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64 * scale,
                         32 * scale, 0),
        SDL_Deleter());
    renderer = std::unique_ptr<SDL_Renderer, SDL_Deleter>(
        SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED), SDL_Deleter());
    SDL_RenderSetScale(renderer.get(), scale, scale);
    texture = std::unique_ptr<SDL_Texture, SDL_Deleter>(
        SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING, 64,
                          32),
        SDL_Deleter());
}

Renderer::~Renderer() = default;

void Renderer::drawGraphics(std::array<unsigned short, 64 * 32>& frame) {
    SDL_UpdateTexture(texture.get(), NULL, &frame, 128);
    SDL_RenderCopy(renderer.get(), texture.get(), NULL, NULL);
    SDL_RenderPresent(renderer.get());
}

void Renderer::setTitleBar(std::string title) {
    SDL_SetWindowTitle(window.get(), title.c_str());
}

void Renderer::changeSize(signed int diff) {
    scale = (scale + diff > 0) ? (scale + diff) : 1;
    SDL_SetWindowSize(window.get(), 64 * scale, 32 * scale);
    SDL_RenderSetScale(renderer.get(), scale, scale);
}

void Renderer::SDL_Deleter::operator()(SDL_Window* p) const {
    SDL_DestroyWindow(p);
}

void Renderer::SDL_Deleter::operator()(SDL_Renderer* p) const {
    SDL_DestroyRenderer(p);
}

void Renderer::SDL_Deleter::operator()(SDL_Texture* p) const {
    SDL_DestroyTexture(p);
}
