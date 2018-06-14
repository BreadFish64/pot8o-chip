#pragma once
#include <array>
#include <memory>
#include <SDL_scancode.h>

class Chip8;
class Renderer;

class Keypad {
public:
    explicit Keypad(Chip8* chip8);
    ~Keypad();

    // wait until a key is pressed
    unsigned char waitForInput();
    // check if a key is pressed
    bool keyIsPressed(unsigned char key);
    // check input between cycles
    void checkInput();

private:
    // keys used for the Chip8 keypad
    static const std::array<const SDL_Scancode, 0x10> keys;

    Chip8& chip8;

    // pointer to SDL keyboard state
    const std::unique_ptr<const unsigned char> keyboard_state = nullptr;
};
