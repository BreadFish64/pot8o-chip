#pragma once
#include <array>
#include <string>

int power(int base, int exponent);

class Chip8 {
    unsigned short opcode;
    std::array<unsigned char, 4096> memory;
    std::array<unsigned char, 16> V;
    unsigned short I;
    unsigned short pc;
    unsigned char delay_timer;
    unsigned char sound_timer;
    std::array<unsigned short, 16> stack;
    unsigned short sp;
    std::array<unsigned char, 16> key;

public:
    std::array<std::array<bool, 64>, 32> gfx;
    bool vf;

    void initialize();
    void emulateCycle();
    void loadGame(std::string path);

    static const std::array<unsigned char, 80> font;
};
