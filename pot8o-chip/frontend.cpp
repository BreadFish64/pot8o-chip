#include <iostream>
#include <thread>
#include <SDL.h>
#include "chip8.h"

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

void Frontend::mainLoop() {
    SDL_Event* event = nullptr;
    frame_start = std::chrono::steady_clock::now();

    while (SDL_PollEvent(event)) {
        if (event) {
            if (event->type == SDL_WINDOWEVENT)
                changeSize();
        }

        for (char i = 0; i < keys.size(); i++) {
            keypad_state[i] = keyboard_state.get()[keys[i]];
        }

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

        SDL_UpdateTexture(texture.get(), NULL, &framebuffer.load(), 128);
        SDL_RenderCopy(renderer.get(), texture.get(), NULL, NULL);
        SDL_RenderPresent(renderer.get());

        std::this_thread::sleep_until(frame_start += frame_time);
    }
}

uint8_t Frontend::waitForInput() {
    while (true) {
        for (char i = 0; i < keypad_state.size(); i++) {
            if (keypad_state[i])
                return i;
        }
    }
    return UINT8_MAX;
}

bool Frontend::keyIsPressed(uint8_t key) {
    return keypad_state[key];
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
