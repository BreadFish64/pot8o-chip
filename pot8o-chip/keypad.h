#pragma once
#include <array>
#include <SDL_scancode.h>

class Keypad {
public:
    explicit Keypad();
    ~Keypad();

    // void updateLoop();
    unsigned char waitForInput();
    bool keyIsPressed(unsigned char key);

private:
    static const std::array<SDL_Scancode, 0x10> keys;

    const unsigned char* keyboard_state = nullptr;
    std::array<const unsigned char*, 0x10> keypad_state;
};
