#include <SDL.h>
#include "keypad.h"

Keypad::Keypad() {}

Keypad::~Keypad(){};

unsigned char Keypad::waitForInput() {
    int* numkeys = nullptr;
    const unsigned char* state;
    while (true) {
        state = SDL_GetKeyboardState(numkeys);
        for (char i = 0; i < 16; i++) {
            unsigned char key_num = static_cast<unsigned char>(keys[i]);
            bool is = state[key_num];
            if (is)
                return i;
        }
    }
}

const std::array<SDL_Scancode, 0x10> Keypad::keys{
    SDL_SCANCODE_KP_0,     SDL_SCANCODE_KP_1,    SDL_SCANCODE_KP_2,      SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,     SDL_SCANCODE_KP_5,    SDL_SCANCODE_KP_6,      SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,     SDL_SCANCODE_KP_9,    SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_ENTER,  SDL_SCANCODE_KP_PERIOD};
