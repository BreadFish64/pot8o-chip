#pragma once
#include <array>
#include <mutex>
#include <string>
#include <queue>
#include "keypad.h"

int power(int base, int exponent);

class Chip8 {
public:
    explicit Chip8(std::queue<std::array<std::array<bool, 64>, 32>>& buffer,
                   std::timed_mutex& buffer_mutex);

private:
    static const std::array<unsigned char, 80> font;

    std::queue<std::array<std::array<bool, 64>, 32>>& buffer;
    std::timed_mutex& buffer_mutex;

    Keypad keypad;

    std::array<std::array<bool, 64>, 32> gfx;
    std::array<unsigned short, 16> stack;
    std::array<unsigned char, 0x1000> memory;
    std::array<unsigned char, 16> V;
    std::array<unsigned char, 16> key;
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
