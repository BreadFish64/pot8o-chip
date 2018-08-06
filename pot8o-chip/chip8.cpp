#include <execution>
#include <fstream>
#include <iostream>
#include <istream>
#include <thread>
#include "chip8.h"
#include "dynarec.h"
#include "interpreter.h"

Chip8::Chip8() : frontend(std::make_unique<Frontend>(this)), cpu(std::make_unique<Dynarec>(this)) {
    cycle_length = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double, std::milli>(1000.0 / target_clock_speed));
    std::copy(font.begin(), font.end(), memory.begin());
}

Chip8::~Chip8() = default;

void Chip8::changeSpeed(signed int diff) {
    target_clock_speed = (target_clock_speed + diff > 0) ? (target_clock_speed + diff) : 1;
    cycle_length = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double, std::milli>(1000.0 / target_clock_speed));
    frontend->setTitleBar(title + " - " + std::to_string(target_clock_speed) + "Hz");
}

void Chip8::loadGame(std::string path) {
    if (path.at(path.size() - 1) == '"')
        path.erase(path.size() - 1);
    if (path.at(0) == '"')
        path.erase(0, 1);
    title = path.substr(path.find_last_of('/') + 1, path.size() - path.find_last_of('/' - 1));
    title = title.substr(title.find_last_of('\\') + 1, title.size() - title.find_last_of('\\') - 1);
    frontend->setTitleBar(title + " - " + std::to_string(target_clock_speed) + "Hz");
    std::ifstream file(path, std::ios::binary);
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(),
              memory.begin() + 0x200);
}

void Chip8::emulate() {
    cpu->execute();
}

uint32_t* Chip8::getFrameBuffer() {
    return frame_buffer.data();
}

std::mutex& Chip8::getFrameBufferLock() {
    return frame_buffer_lock;
}

Opcode::Opcode(uint16_t opcode) : opcode(opcode) {}
Opcode::Opcode() = default;
Opcode::~Opcode() = default;

uint8_t Opcode::op() {
    return (opcode & 0xF000) >> 12;
}

uint8_t Opcode::X() {
    return (opcode & 0x0F00) >> 8;
}

uint8_t Opcode::Y() {
    return (opcode & 0x00F0) >> 4;
}

uint8_t Opcode::n() {
    return opcode & 0x000F;
}

uint8_t Opcode::kk() {
    return opcode & 0x00FF;
}

uint16_t Opcode::nnn() {
    return opcode & 0x0FFF;
}

// clang-format off
const std::array< uint8_t, 0x50> Chip8::font{
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
};

