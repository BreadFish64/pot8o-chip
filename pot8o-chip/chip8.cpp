#include <execution>
#include <fstream>
#include <iostream>
#include <istream>
#include <thread>
#include "chip8.h"

Chip8::Chip8() : frontend(std::make_unique<Frontend>(this)) {
    cycle_length = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double, std::milli>(1000.0 / target_clock_speed));
}

Chip8::~Chip8() = default;

void Chip8::changeSpeed(signed int diff) {
    target_clock_speed = (target_clock_speed + diff > 0) ? (target_clock_speed + diff) : 1;
    cycle_length = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double, std::milli>(1000.0 / target_clock_speed));
    frontend->setTitleBar(title + " - " + std::to_string(target_clock_speed) + "Hz");
}

void Chip8::initialize() {
    rng.seed(std::random_device()());

    program_counter = 0x200;
    opcode = 0;
    I = 0;
    frame_buffer_lock.lock();
    std::fill(frame_buffer.begin(), frame_buffer.end(), 0);
    frame_buffer_lock.unlock();
    std::fill(V.begin(), V.end(), 0);
    std::fill(memory.begin(), memory.end(), 0);

    stack.clear();

    std::copy(font.begin(), font.end(), memory.begin());

    delay_timer = 0;
    sound_timer = 0;
}

void Chip8::loadGame(std::string path) {
    initialize();
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
    cycle_start = std::chrono::steady_clock::now();
    while (true) {
        if (!paused)
            emulateCycle();
        cycle_count++;
        if (limitSpeed)
            std::this_thread::sleep_until(cycle_start += cycle_length);
    }
}

void Chip8::emulateCycle() {
    opcode = memory[program_counter] << 8 | memory[program_counter + 1];
    cpu.execute();

    if (delay_timer > 0)
        --delay_timer;

    if (sound_timer > 0) {
        --sound_timer;
        if (!sound_timer)
            printf("BEEP\n");
    }
}

uint32_t* Chip8::getFrameBuffer() {
    return frame_buffer.data();
}

std::mutex& Chip8::getFrameBufferLock() {
    return frame_buffer_lock;
}

inline uint8_t Chip8::op() {
    return (opcode & 0xF000) >> 12;
}

inline uint8_t Chip8::X() {
    return (opcode & 0x0F00) >> 8;
}

inline uint8_t Chip8::Y() {
    return (opcode & 0x00F0) >> 4;
}

inline uint8_t& Chip8::Vx() {
    return V[X()];
}

inline uint8_t& Chip8::Vy() {
    return V[Y()];
}

inline uint8_t Chip8::n() {
    return opcode & 0x000F;
}

inline uint8_t Chip8::kk() {
    return opcode & 0x00FF;
}

inline uint16_t Chip8::nnn() {
    return opcode & 0x0FFF;
}

Chip8::CPU::CPU(Chip8* parent) : system(*parent) {}

Chip8::CPU::~CPU() = default;

inline void Chip8::CPU::execute() {
    opcode_table[system.op()](*this);
}

inline void Chip8::CPU::step() {
    system.program_counter += 2;
}

void Chip8::CPU::split_0() {
    opcode_table_0[system.kk()](*this);
}

void Chip8::CPU::CLS() {
    system.frame_buffer_lock.lock();
    std::fill(std::execution::par_unseq, system.frame_buffer.begin(), system.frame_buffer.end(), 0);
    system.frame_buffer_lock.unlock();
    step();
}

void Chip8::CPU::RET() {
    system.program_counter = system.stack.back() + 2;
    system.stack.pop_back();
}

void Chip8::CPU::JP_addr() {
    system.program_counter = system.nnn();
}

void Chip8::CPU::CALL_addr() {
    system.stack.push_back(system.program_counter);
    system.program_counter = system.nnn();
}

void Chip8::CPU::SE_Vx_byte() {
    system.program_counter += system.Vx() == system.kk() ? 4 : 2;
}

void Chip8::CPU::SNE_Vx_byte() {
    system.program_counter += system.Vx() != system.kk() ? 4 : 2;
}

void Chip8::CPU::SE_Vx_Vy() {
    system.program_counter += system.Vx() == system.Vy() ? 4 : 2;
}

void Chip8::CPU::LD_Vx_byte() {
    system.Vx() = system.kk();
    step();
}

void Chip8::CPU::ADD_Vx_byte() {
    system.Vx() += system.kk();
    step();
}

void Chip8::CPU::split_8() {
    opcode_table_8[system.n()](*this);
}

void Chip8::CPU::LD_Vx_Vy() {
    system.Vx() = system.Vy();
    step();
}

void Chip8::CPU::OR_Vx_Vy() {
    system.Vx() |= system.Vy();
    step();
}

void Chip8::CPU::AND_Vx_Vy() {
    system.Vx() &= system.Vy();
    step();
}

void Chip8::CPU::XOR_Vx_Vy() {
    system.Vx() ^= system.Vy();
    step();
}

void Chip8::CPU::ADD_Vx_Vy() {
    uint16_t result = system.Vx() + system.Vy();
    system.V[0xF] = result > 0xFF;
    system.Vx() = static_cast<uint8_t>(result & 0xFF);
    step();
}

void Chip8::CPU::SUB_Vx_Vy() {
    system.V[0xF] = system.Vx() > system.Vy();
    system.Vx() -= system.Vy();
    step();
}

void Chip8::CPU::SHR_Vx() {
    system.V[0xF] = system.Vx() & 0b0000001;
    system.Vx() >>= 1;
    step();
}

void Chip8::CPU::SUBN_Vx_Vy() {
    system.V[0xF] = system.Vy() > system.Vx();
    system.Vx() = system.Vy() - system.Vx();
    step();
}

void Chip8::CPU::SHL_Vx() {
    system.V[0xF] = (system.Vx() & 0b1000000) >> 7;
    system.Vx() <<= 1;
    step();
}

void Chip8::CPU::SNE_Vx_Vy() {
    system.program_counter += system.Vx() != system.Vy() ? 4 : 2;
}

void Chip8::CPU::LD_I_addr() {
    system.I = system.nnn();
    step();
}

void Chip8::CPU::JP_0_addr() {
    system.program_counter = system.nnn() + system.V[0x0];
}

void Chip8::CPU::RND_Vx_byte() {
    system.Vx() = system.dist(system.rng) & system.kk();
    step();
}

void Chip8::CPU::DRW_Vx_Vy_nibble() {
    uint8_t x = system.Vx() + 7;
    uint8_t y = system.Vy();
    uint8_t height = system.n();
    system.V[0xF] = false;

    system.frame_buffer_lock.lock();
    for (uint8_t row = 0; row < height; ++row) {
        uint8_t byte = system.memory[system.I + row];
        for (uint8_t column = 0; column < 8; ++column) {
            uint32_t& pixel = system.frame_buffer[((y & 31) << 6) | ((x - column) & 63)];
            if (byte & (1 << column)) {
                if (!(pixel ^= UINT32_MAX))
                    system.V[0xF] = true;
            }
        }
        ++y;
    }
    system.frame_buffer_lock.unlock();
    step();
}

void Chip8::CPU::split_E() {
    opcode_table_E[system.kk()](*this);
}

void Chip8::CPU::SKP_Vx() {
    system.program_counter += system.frontend->keyIsPressed(system.Vx()) ? 4 : 2;
}

void Chip8::CPU::SKNP_Vx() {
    system.program_counter += system.frontend->keyIsPressed(system.Vx()) ? 2 : 4;
}

void Chip8::CPU::split_F() {
    opcode_table_F[system.kk()](*this);
}

void Chip8::CPU::LD_Vx_DT() {
    system.Vx() = system.delay_timer;
    step();
}

void Chip8::CPU::LD_Vx_K() {
    system.Vx() = system.frontend->waitForInput();
    step();
}

void Chip8::CPU::LD_DT_Vx() {
    system.delay_timer = system.Vx();
    step();
}

void Chip8::CPU::LD_ST_Vx() {
    system.sound_timer = system.Vx();
    step();
}

void Chip8::CPU::ADD_I_Vx() {
    system.I += system.Vx();
    step();
}

void Chip8::CPU::LD_F_Vx() {
    system.I = system.Vx() * 5;
    step();
}

void Chip8::CPU::LD_B_Vx() {
    uint8_t num = system.Vx();
    system.memory[system.I] = num / 100;
    num %= 100;
    system.memory[system.I + 1] = num / 10;
    num %= 10;
    system.memory[system.I + 2] = num;
    step();
}

void Chip8::CPU::LD_I_Vx() {
    std::copy_n(std::execution::par_unseq, system.V.begin(), system.X() + 1,
                system.memory.begin() + system.I);
    step();
}

void Chip8::CPU::LD_Vx_I() {
    std::copy_n(std::execution::par_unseq, system.memory.begin() + system.I, system.X() + 1,
                system.V.begin());
    step();
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

const std::array<std::function<void(Chip8::CPU&)>, 0x10> Chip8::CPU::opcode_table {
    &CPU::split_0,		&CPU::JP_addr,			&CPU::CALL_addr,	&CPU::SE_Vx_byte, 
	&CPU::SNE_Vx_byte,	&CPU::SE_Vx_Vy,			&CPU::LD_Vx_byte,	&CPU::ADD_Vx_byte,
    &CPU::split_8,		&CPU::SNE_Vx_Vy,		&CPU::LD_I_addr,	&CPU::JP_0_addr,
	&CPU::RND_Vx_byte,	&CPU::DRW_Vx_Vy_nibble,	&CPU::split_E,		&CPU::split_F
}; 

const std::array<std::function<void(Chip8::CPU&)>, 0x100> Chip8::CPU::opcode_table_0 {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	&CPU::CLS, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::RET, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

const std::array<std::function<void(Chip8::CPU&)>, 0x10> Chip8::CPU::opcode_table_8 {
    &CPU::LD_Vx_Vy,		&CPU::OR_Vx_Vy,		&CPU::AND_Vx_Vy,	&CPU::XOR_Vx_Vy,
	&CPU::ADD_Vx_Vy,	&CPU::SUB_Vx_Vy,	&CPU::SHR_Vx,		&CPU::SUBN_Vx_Vy, 
	nullptr,			nullptr,			nullptr,			nullptr, 
	nullptr,			nullptr,			&CPU::SHL_Vx,		nullptr
};

const std::array<std::function<void(Chip8::CPU&)>, 0x100> Chip8::CPU::opcode_table_E {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::SKP_Vx, nullptr,
	nullptr, &CPU::SKNP_Vx, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};

const std::array<std::function<void(Chip8::CPU&)>, 0x100> Chip8::CPU::opcode_table_F {
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::LD_Vx_DT, 
	nullptr, nullptr, &CPU::LD_Vx_K, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::LD_DT_Vx, nullptr, nullptr, 
	&CPU::LD_ST_Vx, nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::ADD_I_Vx, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, &CPU::LD_F_Vx, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, &CPU::LD_B_Vx, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::LD_I_Vx, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, &CPU::LD_Vx_I, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, 
	nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
};
// clang-format on
