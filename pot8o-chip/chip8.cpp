#include <fstream>
#include <istream>
#include <vector>
#include "chip8.h"

void Chip8::initialize() {
    // Initialize registers and memory once
    pc = 0x200; // Program counter starts at 0x200
    opcode = 0; // Reset current opcode
    I = 0;      // Reset index register
    sp = 0;     // Reset stack pointer
    vf = 0;

    for (auto& row : gfx) {
        for (bool& column : row) {
            column = false;
        }
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
            for (auto& row : gfx) {
                for (bool& column : row) {
                    column = false;
                }
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
        pc = opcode & 0x0FFF;
        break;

    case 0x3000:
        if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
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

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
        }
        break;

    case 0x9000:
        if (V[(opcode & 0x0F00 >> 8)] != V[(opcode & 0x00F0 >> 4)])
            pc += 2;
        break;

    case 0xA000:
        I = opcode & 0x0FFF;
        break;

    case 0xD000: {
        int x = V[opcode & 0x0F00 >> 8];
        int height = opcode & 0x000F;
        int y = (opcode & 0x00F0) >> 4;
        for (int row = 0; row < height; row++) {
            int byte = memory[I + row];
            for (int i = 0; i < 8; i++) {
                bool pixel = gfx[y + row][x + i];
                gfx[y + row][x + i] = ((byte & power(2, i)) >> i) ^ pixel;
                if (gfx[y + row][x + i] != pixel)
                    vf = true;
            }
        }
        break;
    }

    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x0015:
            delay_timer = V[(opcode & 0x0F00) >> 8];
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
            for (int i = 0; i < V.size(); i++) {
                memory[I + i] = V[i];
            }
            break;

        case 0x0065:
            for (int i = 0; i < V.size(); i++) {
                V[i] = memory[I + i];
            }
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
        }
        break;

    default:
        printf("Unknown opcode: 0x%X\n", opcode);
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
    if (pc >= 4096) {
        pc = 0;
    }
}

void Chip8::loadGame(std::string path) {
    path = "E:/git/chip8_ROMs/TICTAC";
    std::ifstream file(path, std::ios::binary);
    file.seekg(0, file.end);
    int length = file.tellg();
    file.seekg(0, file.beg);

    char* buffer = new char[length];
    file.read(buffer, length);

    for (int i = 0; i < length; ++i)
        memory[i + 512] = buffer[i];
}

const std::array<unsigned char, 0x50> Chip8::font = {
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
