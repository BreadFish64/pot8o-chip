#include <iostream>
#include <SDL.h>
#include "chip8.h"
#include "renderer.h"

Frontend::Frontend(Chip8* chip8)
    : chip8(*chip8), keyboard_state(std::unique_ptr<const uint8_t>(SDL_GetKeyboardState(nullptr))),
      window(std::unique_ptr<SDL_Window, SDL_Deleter>(
          SDL_CreateWindow("pot8o chip", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 64 * 8,
                           32 * 8, SDL_WINDOW_RESIZABLE),
          SDL_Deleter())),
      renderer(std::unique_ptr<SDL_Renderer, SDL_Deleter>(
          SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED), SDL_Deleter())),
      texture(std::unique_ptr<SDL_Texture, SDL_Deleter>(
          SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA4444, SDL_TEXTUREACCESS_STREAMING,
                            64, 32),
          SDL_Deleter())) {
    SDL_RenderSetScale(renderer.get(), 8.0f, 8.0f);
}

Frontend::~Frontend() = default;

uint8_t Frontend::waitForInput() {
    SDL_Event* event = nullptr;
    while (SDL_PollEvent(event)) {
        for (uint8_t i = 0; i < keys.size(); i++) {
            if (keyboard_state.get()[keys[i]])
                return i;
        }
        if (event) {
            if (event->type == SDL_WINDOWEVENT)
                chip8.changeWindowSize();
        }
    }
    exit(1);
}

bool Frontend::keyIsPressed(uint8_t key) {
    SDL_Event* event = nullptr;
    SDL_PollEvent(event);
    if (event) {
        if (event->type == SDL_WINDOWEVENT)
            chip8.changeWindowSize();
    }
    return keyboard_state.get()[keys[key]];
}

void Frontend::checkInput() {
    SDL_Event* event = nullptr;
    SDL_PollEvent(event);

    if (keyboard_state.get()[SDL_SCANCODE_ESCAPE])
        exit(0);
    if (keyboard_state.get()[SDL_SCANCODE_RIGHT] | keyboard_state.get()[SDL_SCANCODE_UP])
        chip8.changeSpeed(1);
    if (keyboard_state.get()[SDL_SCANCODE_LEFT] | keyboard_state.get()[SDL_SCANCODE_DOWN])
        chip8.changeSpeed(-1);
    if (keyboard_state.get()[SDL_SCANCODE_L])
        chip8.limitSpeed = true;
    if (keyboard_state.get()[SDL_SCANCODE_U])
        chip8.limitSpeed = false;
    if (keyboard_state.get()[SDL_SCANCODE_C]) {
        std::string game;
        std::cin >> game;
        chip8.loadGame(game);
    }
    if (keyboard_state.get()[SDL_SCANCODE_P])
        chip8.paused = true;
    if (keyboard_state.get()[SDL_SCANCODE_G])
        chip8.paused = false;

    if (event) {
        if (event->type == SDL_WINDOWEVENT)
            chip8.changeWindowSize();
    }
}

void Frontend::drawGraphics(const std::array<uint16_t, 64 * 32>& frame) {
    SDL_UpdateTexture(texture.get(), NULL, &frame, 128);
    SDL_RenderCopy(renderer.get(), texture.get(), NULL, NULL);
    SDL_RenderPresent(renderer.get());
}

void Frontend::setTitleBar(std::string title) {
    SDL_SetWindowTitle(window.get(), title.c_str());
}

void Frontend::changeSize() {
    std::unique_ptr<int> width, height;
    SDL_GetWindowSize(window.get(), width.get(), height.get());
    SDL_RenderSetScale(renderer.get(), *width.get() / 64.0f, *height.get() / 64.0f);
}

void Frontend::SDL_Deleter::operator()(SDL_Window* p) const {
    SDL_DestroyWindow(p);
}

void Frontend::SDL_Deleter::operator()(SDL_Renderer* p) const {
    SDL_DestroyRenderer(p);
}

void Frontend::SDL_Deleter::operator()(SDL_Texture* p) const {
    SDL_DestroyTexture(p);
}

const std::array<const SDL_Scancode, 0x10> Frontend::keys{
    SDL_SCANCODE_KP_0,     SDL_SCANCODE_KP_1,    SDL_SCANCODE_KP_2,      SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,     SDL_SCANCODE_KP_5,    SDL_SCANCODE_KP_6,      SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,     SDL_SCANCODE_KP_9,    SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_ENTER,  SDL_SCANCODE_KP_PERIOD};
