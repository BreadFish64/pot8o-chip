#pragma once
#include <array>
#include <memory>
#include <SDL_scancode.h>

class Keypad {
public:
    explicit Keypad();
    ~Keypad();

    unsigned char waitForInput();
    bool keyIsPressed(unsigned char key);

private:
    static const std::array<SDL_Scancode, 0x10> keys;

    std::unique_ptr<const unsigned char> keyboard_state = nullptr;
};
