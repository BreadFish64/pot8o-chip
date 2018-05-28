#pragma once
#include <array>
#include <utility>
#include <SDL_scancode.h>

class Keypad {
public:
    explicit Keypad();
    ~Keypad();

    unsigned char waitForInput();

private:
    static const std::array<SDL_Scancode, 0x10> keys;
};
