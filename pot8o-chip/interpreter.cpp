#include <algorithm>
#include <execution>

#include "chip8.h"
#include "interpreter.h"

Interpreter::Interpreter(Chip8* parent) : system(*parent) {
    PC = 0x200;
    rng.seed(std::random_device()());
}

Interpreter::~Interpreter() = default;

void Interpreter::execute() {
    cycle_start = std::chrono::steady_clock::now();
    while (true) {
        opcode = system.memory[PC] << 8 | system.memory[PC | 1];

        opcode_table[opcode.op()](*this);

        if (DT)
            --DT;

        if (ST) {
            --ST;
            if (!ST)
                printf("BEEP\n");
        }

        if (false)
            std::this_thread::sleep_until(cycle_start += system.cycle_length);
        system.cycle_count++;
    }
}

inline void Interpreter::step() {
    PC += 2;
}

inline uint8_t& Interpreter::Vx() {
    return V[opcode.X()];
}

inline uint8_t& Interpreter::Vy() {
    return V[opcode.Y()];
}

void Interpreter::split_0() {
    opcode_table_0[opcode.kk()](*this);
}

void Interpreter::CLS() {
    system.frame_buffer_lock.lock();
    std::fill(std::execution::par_unseq, system.frame_buffer.begin(), system.frame_buffer.end(), 0);
    system.frame_buffer_lock.unlock();
    step();
}

void Interpreter::RET() {
    PC = stack.back() + 2;
    stack.pop_back();
}

void Interpreter::JP_addr() {
    PC = opcode.nnn();
}

void Interpreter::CALL_addr() {
    stack.push_back(PC);
    PC = opcode.nnn();
}

void Interpreter::SE_Vx_byte() {
    PC += Vx() == opcode.kk() ? 4 : 2;
}

void Interpreter::SNE_Vx_byte() {
    PC += Vx() != opcode.kk() ? 4 : 2;
}

void Interpreter::SE_Vx_Vy() {
    PC += Vx() == Vy() ? 4 : 2;
}

void Interpreter::LD_Vx_byte() {
    Vx() = opcode.kk();
    step();
}

void Interpreter::ADD_Vx_byte() {
    Vx() += opcode.kk();
    step();
}

void Interpreter::split_8() {
    opcode_table_8[opcode.n()](*this);
}

void Interpreter::LD_Vx_Vy() {
    Vx() = Vy();
    step();
}

void Interpreter::OR_Vx_Vy() {
    Vx() |= Vy();
    step();
}

void Interpreter::AND_Vx_Vy() {
    Vx() &= Vy();
    step();
}

void Interpreter::XOR_Vx_Vy() {
    Vx() ^= Vy();
    step();
}

void Interpreter::ADD_Vx_Vy() {
    uint16_t result = Vx() + Vy();
    V[0xF] = result > 0xFF;
    Vx() = static_cast<uint8_t>(result & 0xFF);
    step();
}

void Interpreter::SUB_Vx_Vy() {
    V[0xF] = Vx() > Vy();
    Vx() -= Vy();
    step();
}

void Interpreter::SHR_Vx() {
    V[0xF] = Vx() & 0b0000001;
    Vx() >>= 1;
    step();
}

void Interpreter::SUBN_Vx_Vy() {
    V[0xF] = Vy() > Vx();
    Vx() = Vy() - Vx();
    step();
}

void Interpreter::SHL_Vx() {
    V[0xF] = (Vx() & 0b1000000) >> 7;
    Vx() <<= 1;
    step();
}

void Interpreter::SNE_Vx_Vy() {
    PC += Vx() != Vy() ? 4 : 2;
}

void Interpreter::LD_I_addr() {
    I = opcode.nnn();
    step();
}

void Interpreter::JP_0_addr() {
    PC = opcode.nnn() + V[0x0];
}

void Interpreter::RND_Vx_byte() {
    Vx() = dist(rng) & opcode.kk();
    step();
}

void Interpreter::DRW_Vx_Vy_nibble() {
    uint8_t x = Vx() + 7;
    uint8_t y = Vy();
    uint8_t height = opcode.n();
    V[0xF] = false;

    system.frame_buffer_lock.lock();
    for (uint8_t row = 0; row < height; ++row) {
        uint8_t byte = system.memory[I + row];
        for (uint8_t column = 0; column < 8; ++column) {
            uint32_t& pixel = system.frame_buffer[((y & 31) << 6) | ((x - column) & 63)];
            if (byte & (1 << column)) {
                if (!(pixel ^= UINT32_MAX))
                    V[0xF] = true;
            }
        }
        ++y;
    }
    system.frame_buffer_lock.unlock();
    step();
}

void Interpreter::split_E() {
    opcode_table_E[opcode.kk()](*this);
}

void Interpreter::SKP_Vx() {
    PC += system.frontend->keyIsPressed(Vx()) ? 4 : 2;
}

void Interpreter::SKNP_Vx() {
    PC += system.frontend->keyIsPressed(Vx()) ? 2 : 4;
}

void Interpreter::split_F() {
    opcode_table_F[opcode.kk()](*this);
}

void Interpreter::LD_Vx_DT() {
    Vx() = DT;
    step();
}

void Interpreter::LD_Vx_K() {
    Vx() = system.frontend->waitForInput();
    step();
}

void Interpreter::LD_DT_Vx() {
    DT = Vx();
    step();
}

void Interpreter::LD_ST_Vx() {
    ST = Vx();
    step();
}

void Interpreter::ADD_I_Vx() {
    I += Vx();
    step();
}

void Interpreter::LD_F_Vx() {
    I = Vx() * 5;
    step();
}

void Interpreter::LD_B_Vx() {
    uint8_t num = Vx();
    system.memory[I] = num / 100;
    num %= 100;
    system.memory[I + 1] = num / 10;
    num %= 10;
    system.memory[I + 2] = num;
    step();
}

void Interpreter::LD_I_Vx() {
    std::copy_n(std::execution::par_unseq, V.begin(), opcode.X() + 1, system.memory.begin() + I);
    step();
}

void Interpreter::LD_Vx_I() {
    std::copy_n(std::execution::par_unseq, system.memory.begin() + I, opcode.X() + 1, V.begin());
    step();
}

// clang-format off
const std::array<std::function<void(Interpreter&)>, 0x10> Interpreter::opcode_table{
    &Interpreter::split_0,     &Interpreter::JP_addr,          &Interpreter::CALL_addr,  &Interpreter::SE_Vx_byte,
    &Interpreter::SNE_Vx_byte, &Interpreter::SE_Vx_Vy,         &Interpreter::LD_Vx_byte, &Interpreter::ADD_Vx_byte,
    &Interpreter::split_8,     &Interpreter::SNE_Vx_Vy,        &Interpreter::LD_I_addr,  &Interpreter::JP_0_addr,
    &Interpreter::RND_Vx_byte, &Interpreter::DRW_Vx_Vy_nibble, &Interpreter::split_E,    &Interpreter::split_F
};

const std::array<std::function<void(Interpreter&)>, 0x100> Interpreter::opcode_table_0{
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, &Interpreter::CLS, nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, &Interpreter::RET, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr, nullptr, nullptr, nullptr,   nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr,   nullptr,
};

const std::array<std::function<void(Interpreter&)>, 0x10> Interpreter::opcode_table_8{
    &Interpreter::LD_Vx_Vy,	&Interpreter::OR_Vx_Vy,	&Interpreter::AND_Vx_Vy,&Interpreter::XOR_Vx_Vy,
    &Interpreter::ADD_Vx_Vy,&Interpreter::SUB_Vx_Vy,&Interpreter::SHR_Vx,	&Interpreter::SUBN_Vx_Vy,
    nullptr,				nullptr,				nullptr,				nullptr,
    nullptr,				nullptr,				&Interpreter::SHL_Vx,	nullptr
};

const std::array<std::function<void(Interpreter&)>, 0x100> Interpreter::opcode_table_E{
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, &Interpreter::SKP_Vx, nullptr, nullptr, &Interpreter::SKNP_Vx,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,      nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, nullptr,
};

const std::array<std::function<void(Interpreter&)>, 0x100> Interpreter::opcode_table_F{
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    &Interpreter::LD_Vx_DT, nullptr,       nullptr,        &Interpreter::LD_Vx_K,  nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    &Interpreter::LD_DT_Vx, nullptr,       nullptr,        &Interpreter::LD_ST_Vx, nullptr, nullptr, nullptr,
    nullptr,        nullptr,       &Interpreter::ADD_I_Vx, nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, &Interpreter::LD_F_Vx,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       &Interpreter::LD_B_Vx,  nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        &Interpreter::LD_I_Vx, nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        &Interpreter::LD_Vx_I,  nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,        nullptr, nullptr, nullptr,
    nullptr,        nullptr,       nullptr,        nullptr,
};
// clang-format on
