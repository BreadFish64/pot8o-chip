#include <execution>
#include <fstream>
#include <iostream>
#include <istream>
#include <thread>
#include "chip8.h"
#include "dynarec/dynarec.h"
#include "dynarec/enums.h"
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
    if (path.at(path.size() - 1) == '"') path.erase(path.size() - 1);
    if (path.at(0) == '"') path.erase(0, 1);
    title = path.substr(path.find_last_of('/') + 1, path.size() - path.find_last_of('/' - 1));
    title = title.substr(title.find_last_of('\\') + 1, title.size() - title.find_last_of('\\') - 1);
    frontend->setTitleBar(title + " - " + std::to_string(target_clock_speed) + "Hz");
    std::ifstream file(path, std::ios::binary);
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(),
              memory.begin() + 0x200);
}

void Chip8::emulate() { cpu->execute(); }

uint32_t* Chip8::getFrameBuffer() { return frame_buffer.data(); }

std::mutex& Chip8::getFrameBufferLock() { return frame_buffer_lock; }

Opcode::Opcode(uint16_t opcode) : opcode(opcode) {}
Opcode::Opcode() = default;
Opcode::~Opcode() = default;

uint8_t Opcode::op() { return (opcode & 0xF000) >> 12; }

uint8_t Opcode::X() { return (opcode & 0x0F00) >> 8; }

uint8_t Opcode::Y() { return (opcode & 0x00F0) >> 4; }

uint8_t Opcode::n() { return opcode & 0x000F; }

uint8_t Opcode::kk() { return opcode & 0x00FF; }

uint16_t Opcode::nnn() { return opcode & 0x0FFF; }

int Opcode::getInstruction() {
    using namespace Instructions;
    try {
        switch (op()) {
        case 0x0:
            switch (kk()) {
            case 0xE0: return CLS;
            case 0xEE: return RET;
            default: throw opcode;
            }
        case 0x1: return JP_addr;
        case 0x2: return CALL_addr;
        case 0x3: return SE_Vx_byte;
        case 0x4: return SNE_Vx_byte;
        case 0x5: return SE_Vx_Vy;
        case 0x6: return LD_Vx_byte;
        case 0x7: return ADD_Vx_byte;
        case 0x8:
            switch (n()) {
            case 0x0: return LD_Vx_Vy;
            case 0x1: return OR_Vx_Vy;
            case 0x2: return AND_Vx_Vy;
            case 0x3: return XOR_Vx_Vy;
            case 0x4: return ADD_Vx_Vy;
            case 0x5: return SUB_Vx_Vy;
            case 0x6: return SHR_Vx;
            case 0x7: return SUBN_Vx_Vy;
            case 0xE: return SHL_Vx;
            default: throw opcode;
            }
        case 0x9: return SNE_Vx_Vy;
        case 0xA: return LD_I_addr;
        case 0xB: return JP_V0_addr;
        case 0xC: return RND_Vx_byte;
        case 0xD: return DRW_Vx_Vy_nibble;
        case 0xE:
            switch (kk()) {
            case 0x9E: SKP_Vx;
            case 0xA1: SKNP_Vx;
            default: throw opcode;
            }
        case 0xF:
            switch (kk()) {
            case 0x07: return LD_Vx_DT;
            case 0x0A: return LD_Vx_K;
            case 0x15: return LD_DT_Vx;
            case 0x18: return LD_ST_Vx;
            case 0x1E: return ADD_I_Vx;
            case 0x29: return LD_F_Vx;
            case 0x33: return LD_B_Vx;
            case 0x55: return LD_I_Vx;
            case 0x65: return LD_Vx_I;
            default: throw opcode;
            }
        default: throw opcode;
        }
    } catch (int e) {
        printf("unexpected opcode: %#6x\n", e);
        return INVALID_OPCODE;
    }
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

