#pragma once
#include <array>
#include <memory>
#include <SDL_scancode.h>

class Chip8;
class Renderer;

class Keypad {
public:
    explicit Keypad(Chip8* chip8, Renderer* renderer);
    ~Keypad();

    unsigned char waitForInput();
    bool keyIsPressed(unsigned char key);
    void checkInput();

private:
    static const std::array<const SDL_Scancode, 0x10> keys;

    Chip8& chip8;
    Renderer& renderer;
    std::unique_ptr<const unsigned char> keyboard_state = nullptr;
};
