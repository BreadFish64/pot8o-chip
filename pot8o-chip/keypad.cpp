#include <algorithm>
#include <SDL.h>
#include "keypad.h"

Keypad::Keypad() {
    keyboard_state = SDL_GetKeyboardState(nullptr);
}

Keypad::~Keypad(){};

unsigned char Keypad::waitForInput() {
    while (SDL_PollEvent(nullptr)) {
        for (unsigned char i = 0; i < keys.size(); i++) {
            if (keyboard_state[keys[i]])
                return i;
        }
    }
    exit(1);
}

bool Keypad::keyIsPressed(unsigned char key) {
    SDL_PollEvent(nullptr);

    return keyboard_state[keys[key]];
}

const std::array<SDL_Scancode, 0x10> Keypad::keys{
    SDL_SCANCODE_KP_0,     SDL_SCANCODE_KP_1,    SDL_SCANCODE_KP_2,      SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,     SDL_SCANCODE_KP_5,    SDL_SCANCODE_KP_6,      SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,     SDL_SCANCODE_KP_9,    SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_ENTER,  SDL_SCANCODE_KP_PERIOD};
