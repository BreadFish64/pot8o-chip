#include <iostream>
#include <SDL.h>
#include "chip8.h"
#include "keypad.h"
#include "renderer.h"

Keypad::Keypad(Chip8* chip8)
    : chip8(*chip8),
      keyboard_state(std::unique_ptr<const unsigned char>(SDL_GetKeyboardState(nullptr))) {}

Keypad::~Keypad() = default;

unsigned char Keypad::waitForInput() {
    SDL_Event* event = nullptr;
    while (SDL_PollEvent(event)) {
        for (unsigned char i = 0; i < keys.size(); i++) {
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

bool Keypad::keyIsPressed(unsigned char key) {
    SDL_Event* event = nullptr;
    SDL_PollEvent(event);
    if (event) {
        if (event->type == SDL_WINDOWEVENT)
            chip8.changeWindowSize();
    }
    return keyboard_state.get()[keys[key]];
}

void Keypad::checkInput() {
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

const std::array<const SDL_Scancode, 0x10> Keypad::keys{
    SDL_SCANCODE_KP_0,     SDL_SCANCODE_KP_1,    SDL_SCANCODE_KP_2,      SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,     SDL_SCANCODE_KP_5,    SDL_SCANCODE_KP_6,      SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,     SDL_SCANCODE_KP_9,    SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_ENTER,  SDL_SCANCODE_KP_PERIOD};
