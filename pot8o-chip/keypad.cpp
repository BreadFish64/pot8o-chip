#include <iostream>
#include <SDL.h>
#include "chip8.h"
#include "keypad.h"
#include "renderer.h"

Keypad::Keypad(Chip8* chip8, Renderer* renderer) : chip8(*chip8), renderer(*renderer) {
    keyboard_state.reset(SDL_GetKeyboardState(nullptr));
}

Keypad::~Keypad() = default;

unsigned char Keypad::waitForInput() {
    while (SDL_PollEvent(nullptr)) {
        for (unsigned char i = 0; i < keys.size(); i++) {
            if (keyboard_state.get()[keys[i]])
                return i;
        }
    }
    exit(1);
}

bool Keypad::keyIsPressed(unsigned char key) {
    SDL_PollEvent(nullptr);
    return keyboard_state.get()[keys[key]];
}

void Keypad::checkInput() {
    SDL_PollEvent(nullptr);
    if (keyboard_state.get()[SDL_SCANCODE_ESCAPE])
        exit(0);
    if (keyboard_state.get()[SDL_SCANCODE_RIGHT])
        chip8.changeSpeed(1);
    if (keyboard_state.get()[SDL_SCANCODE_LEFT])
        chip8.changeSpeed(-1);
    if (keyboard_state.get()[SDL_SCANCODE_UP])
        renderer.changeSize(1);
    if (keyboard_state.get()[SDL_SCANCODE_DOWN])
        renderer.changeSize(-1);
    if (keyboard_state.get()[SDL_SCANCODE_L]) {
        std::string game;
        std::cin >> game;
        chip8.loadGame(game);
    }
    if (keyboard_state.get()[SDL_SCANCODE_U])
        chip8.limitSpeed ^= 1;
}

const std::array<const SDL_Scancode, 0x10> Keypad::keys{
    SDL_SCANCODE_KP_0,     SDL_SCANCODE_KP_1,    SDL_SCANCODE_KP_2,      SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,     SDL_SCANCODE_KP_5,    SDL_SCANCODE_KP_6,      SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,     SDL_SCANCODE_KP_9,    SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_ENTER,  SDL_SCANCODE_KP_PERIOD};
