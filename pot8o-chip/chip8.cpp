#include <fstream>
#include <istream>
#include <random>
#include <vector>
#include "chip8.h"
#include "keypad.h"
#include "render.h"

using namespace std::chrono_literals;

Chip8::Chip8() {
    keypad = new Keypad();
    render = new Render();
    initialize();
    loadGame("meep");
    emulate();
}

void Chip8::initialize() {
    // Initialize registers and memory once
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0;      // Reset index register
    sp = 0;     // Reset stack pointer
    V[0xF] = false;

    for (auto& pixel : gfx) {
        pixel = false;
    } // Clear display
    for (auto& addr : stack) {
        addr = 0;
    } // Clear stack
    for (auto& reg : V) {
        reg = 0;
    } // Clear registers V0-VF
    for (auto& byte : memory) {
        byte = 0;
    } // Clear memory

    // Load fontset
    for (int i = 0; i < 0x50; ++i)
        memory[i] = font[i];

    // Reset timers
    delay_timer = 0;
    sound_timer = 0;
}

void Chip8::emulate() {
    while (true) {
        frame_start = std::chrono::steady_clock::now();
        emulateCycle();
        std::this_thread::sleep_until(frame_start + 16.7ms);
    }
}

void Chip8::emulateCycle() {
    // Fetch Opcode
    opcode = memory[pc] << 8 | memory[pc + 1];
    // Decode Opcode
    // Execute Opcode
    switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode & 0x00FF) {
        case 0x00E0:
            for (auto& pixel : gfx) {
                pixel = false;
            }
            break;

        case 0x00EE:
            pc = stack[sp];
            sp--;
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
        }
        break;

    case 0x1000:
        pc = (opcode & 0x0FFF) - 2;
        break;

    case 0x2000:
        sp++;
        stack[sp] = pc;
        pc = (opcode & 0x0FFF) - 2;
        break;

    case 0x3000:
        if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
            pc += 2;
        break;

    case 0x4000:
        if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
            pc += 2;
        break;

    case 0x6000:
        V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
        break;

    case 0x7000:
        V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
        break;

    case 0x8000:
        switch (opcode & 0x000F) {
        case 0x0000:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
            break;

        case 0x0002:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x0F00) >> 8] & V[(opcode & 0x00F0) >> 4];
            break;

        case 0x0003:
            V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
            break;

        case 0x0004: {
            unsigned short result = V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4];
            V[0xF] = result > 0xFF;
            V[(opcode & 0x0F00) >> 8] = static_cast<unsigned char>(result & 0xFF);
            break;
        }
        case 0x0005: {
            V[0xF] = V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4];
            unsigned short result = V[(opcode & 0x0F00) >> 8] - V[(opcode & 0x00F0) >> 4];
            V[(opcode & 0x0F00) >> 8] = static_cast<unsigned char>(result & 0xFF);
            break;
        }
        case 0x0006:
            V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0b0000001) == 0b0000001;
            V[(opcode & 0x0F00) >> 8] >>= 1;
            break;

        case 0x000E:
            V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0b1000000) == 0b1000000;
            V[(opcode & 0x0F00) >> 8] <<= 1;
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            break;
        }
        break;

    case 0x9000:
        if (V[(opcode & 0x0F00 >> 8)] != V[(opcode & 0x00F0 >> 4)])
            pc += 2;
        break;

    case 0xA000:
        I = opcode & 0x0FFF;
        break;
    case 0xC000: {
        std::random_device seeder;
        std::mt19937 engine(seeder());
        std::uniform_int_distribution<int> dist(CHAR_MIN, CHAR_MAX);
        unsigned char rand_num = dist(engine);
        V[(opcode & 0x0F00 >> 8)] = (opcode & 0x00FF) & rand_num;
        break;
    }
    case 0xD000: {
        unsigned char x = V[(opcode & 0x0F00) >> 8];
        unsigned char height = opcode & 0x000F;
        unsigned char y = V[(opcode & 0x00F0) >> 4];
        V[0xF] = false;

        for (unsigned char row = 0; row < height; row++) {
            unsigned char byte = memory[I + row];
            for (char i = 0; i < 8; i++) {
                unsigned short& pixel = gfx[((y + row) % 32) * 64 + (x + i) % 64];
                bool flip = byte & (1 << i);
                if (pixel && flip)
                    V[0xF] = true;
                if (flip)
                    pixel = ~pixel;
            }
        }

        render->drawGraphics(gfx);
        break;
    }
    case 0xE000:
        switch (opcode & 0x00FF) {
        case 0x009E:
            if (keypad->keyIsPressed((opcode & 0x0F00) >> 8))
                pc += 2;
            break;

        case 0x00A1:
            if (!keypad->keyIsPressed((opcode & 0x0F00) >> 8))
                pc += 2;
            break;
        }
        break;

    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x0007:
            V[(opcode & 0x0F00) >> 8] = delay_timer;
            break;

        case 0x000A:
            V[(opcode & 0x0F00) >> 8] = keypad->waitForInput();
            break;

        case 0x0015:
            delay_timer = V[(opcode & 0x0F00) >> 8];
            break;

        case 0x0018:
            sound_timer = V[(opcode & 0x0F00) >> 8];
            break;

        case 0x001E:
            I += V[(opcode & 0x0F00) >> 8];
            break;

        case 0x0029:
            I = V[(opcode & 0x0F00) >> 8];
            break;

        case 0x0033: {
            unsigned char num = V[(opcode & 0x0F00) >> 8];
            unsigned char cent = num / 100;
            num -= cent;
            unsigned char dec = num / 10;
            num -= dec;
            unsigned char unit = num;
            memory[I] = cent;
            memory[I + 1] = dec;
            memory[I + 2] = unit;
            break;
        }
        case 0x0055:
            for (int i = 0; i <= (opcode & 0x0F00) >> 8; i++) {
                memory[I + i] = V[i];
            }
            break;

        case 0x0065:
            for (int i = 0; i <= (opcode & 0x0F00) >> 8; i++) {
                V[i] = memory[I + i];
            }
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            break;
        }
        break;

    default:
        printf("Unknown opcode: 0x%X\n", opcode);
        break;
    }

    pc += 2;

    // Update timers
    if (delay_timer > 0)
        --delay_timer;

    if (sound_timer > 0) {
        if (sound_timer == 1)
            printf("BEEP!\n");
        --sound_timer;
    }
}

void Chip8::loadGame(std::string path) {
    path = "F:/git/chip8/TICTAC";
    std::ifstream file(path, std::ios::binary);
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(),
              memory.begin() + 512);
}

const std::array<unsigned char, 0x50> Chip8::font{
    // clang-format off
	//0
	0b11110000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11110000,
	//1
	0b00100000,
	0b01100000,
	0b00100000,
	0b00100000,
	0b01110000,
	//2
	0b11110000,
	0b00010000,
	0b11110000,
	0b10000000,
	0b11110000,
	//3
	0b11110000,
	0b00010000,
	0b11110000,
	0b00010000,
	0b11110000,
	//4
	0b10010000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b00010000,
	//5
	0b11110000,
	0b10000000,
	0b11110000,
	0b00010000,
	0b11110000,
	//6
	0b11110000,
	0b10000000,
	0b11110000,
	0b10010000,
	0b11110000,
	//7
	0b11110000,
	0b00010000,
	0b00100000,
	0b01000000,
	0b01000000,
	//8
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b11110000,
	//9
	0b11110000,
	0b10010000,
	0b11110000,
	0b00010000,
	0b11110000,
	//A
	0b11110000,
	0b10010000,
	0b11110000,
	0b10010000,
	0b10010000,
	//B
	0b11100000,
	0b10010000,
	0b11100000,
	0b10010000,
	0b11100000,
	//C
	0b11110000,
	0b10000000,
	0b10000000,
	0b10000000,
	0b11110000,
	//D
	0b11100000,
	0b10010000,
	0b10010000,
	0b10010000,
	0b11100000,
	//E
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b11110000,
	//F
	0b11110000,
	0b10000000,
	0b11110000,
	0b10000000,
	0b10000000,
    // clang-format on
};

int power(int base, int exponent) {
    int result = 1;
    for (int i = 0; i < exponent; i++) {
        result = result * base;
    }
    return result;
}
