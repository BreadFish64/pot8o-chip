#pragma once
#include <array>
#include <chrono>
#include <string>

class Keypad;
class Render;

int power(int base, int exponent);

class Chip8 {
public:
    explicit Chip8();

private:
    static const std::array<unsigned char, 80> font;

    Render* render = nullptr;
    Keypad* keypad = nullptr;

    std::chrono::time_point<std::chrono::steady_clock> frame_start;

    std::array<unsigned short, 64 * 32> gfx;
    std::array<unsigned short, 16> stack;
    std::array<unsigned char, 0x1000> memory;
    std::array<unsigned char, 16> V;
    unsigned short opcode;
    unsigned short I;
    unsigned short pc;
    unsigned short sp;
    unsigned char delay_timer;
    unsigned char sound_timer;

    void initialize();
    void emulate();
    void emulateCycle();
    void loadGame(std::string path);
};
