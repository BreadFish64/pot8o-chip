#include <algorithm>
#include <execution>

#include "dynarec.h"

Dynarec::Dynarec(Chip8* parent) : system(*parent), PC(0x200) {}

Dynarec::~Dynarec() = default;

void Dynarec::execute() {
    while (true) {
        Opcode opcode(system.memory[PC] << 8 | system.memory[PC | 1]);

        opcode_table[opcode.op()](*this, opcode);

        if (DT)
            --DT;

        if (ST) {
            --ST;
            if (!ST)
                printf("BEEP\n");
        }

        system.cycle_count++;
    }
}

inline void Dynarec::step() {
    PC += 2;
}

inline uint8_t& Dynarec::Vx(Opcode& opcode) {
    return V[opcode.X()];
}

inline uint8_t& Dynarec::Vy(Opcode& opcode) {
    return V[opcode.Y()];
}

void Dynarec::split_0(Opcode& opcode) {
    opcode_table_0.at(opcode.kk())(*this, opcode);
}

void Dynarec::CLS(Opcode& opcode) {
    system.frame_buffer_lock.lock();
    std::fill(std::execution::par_unseq, system.frame_buffer.begin(), system.frame_buffer.end(), 0);
    system.frame_buffer_lock.unlock();
    step();
}

void Dynarec::RET(Opcode& opcode) {
    PC = stack.back() + 2;
    stack.pop_back();
}

void Dynarec::JP_addr(Opcode& opcode) {
    PC = opcode.nnn();
}

void Dynarec::CALL_addr(Opcode& opcode) {
    stack.push_back(PC);
    PC = opcode.nnn();
}

void Dynarec::SE_Vx_byte(Opcode& opcode) {
    PC += Vx(opcode) == opcode.kk() ? 4 : 2;
}

void Dynarec::SNE_Vx_byte(Opcode& opcode) {
    PC += Vx(opcode) != opcode.kk() ? 4 : 2;
}

void Dynarec::SE_Vx_Vy(Opcode& opcode) {
    PC += Vx(opcode) == Vy(opcode) ? 4 : 2;
}

void Dynarec::LD_Vx_byte(Opcode& opcode) {
    Vx(opcode) = opcode.kk();
    step();
}

void Dynarec::ADD_Vx_byte(Opcode& opcode) {
    Vx(opcode) += opcode.kk();
    step();
}

void Dynarec::split_8(Opcode& opcode) {
    opcode_table_8[opcode.n()](*this, opcode);
}

void Dynarec::LD_Vx_Vy(Opcode& opcode) {
    Vx(opcode) = Vy(opcode);
    step();
}

void Dynarec::OR_Vx_Vy(Opcode& opcode) {
    Vx(opcode) |= Vy(opcode);
    step();
}

void Dynarec::AND_Vx_Vy(Opcode& opcode) {
    Vx(opcode) &= Vy(opcode);
    step();
}

void Dynarec::XOR_Vx_Vy(Opcode& opcode) {
    Vx(opcode) ^= Vy(opcode);
    step();
}

void Dynarec::ADD_Vx_Vy(Opcode& opcode) {
    uint16_t result = Vx(opcode) + Vy(opcode);
    V[0xF] = result > 0xFF;
    Vx(opcode) = static_cast<uint8_t>(result & 0xFF);
    step();
}

void Dynarec::SUB_Vx_Vy(Opcode& opcode) {
    V[0xF] = Vx(opcode) > Vy(opcode);
    Vx(opcode) -= Vy(opcode);
    step();
}

void Dynarec::SHR_Vx(Opcode& opcode) {
    V[0xF] = Vx(opcode) & 0b0000001;
    Vx(opcode) >>= 1;
    step();
}

void Dynarec::SUBN_Vx_Vy(Opcode& opcode) {
    V[0xF] = Vy(opcode) > Vx(opcode);
    Vx(opcode) = Vy(opcode) - Vx(opcode);
    step();
}

void Dynarec::SHL_Vx(Opcode& opcode) {
    V[0xF] = (Vx(opcode) & 0b1000000) >> 7;
    Vx(opcode) <<= 1;
    step();
}

void Dynarec::SNE_Vx_Vy(Opcode& opcode) {
    PC += Vx(opcode) != Vy(opcode) ? 4 : 2;
}

void Dynarec::LD_I_addr(Opcode& opcode) {
    I = opcode.nnn();
    step();
}

void Dynarec::JP_0_addr(Opcode& opcode) {
    PC = opcode.nnn() + V[0x0];
}

void Dynarec::RND_Vx_byte(Opcode& opcode) {
    Vx(opcode) = rand() & opcode.kk();
    step();
}

void Dynarec::DRW_Vx_Vy_nibble(Opcode& opcode) {
    uint8_t x = Vx(opcode) + 7;
    uint8_t y = Vy(opcode);
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

void Dynarec::split_E(Opcode& opcode) {
    opcode_table_E.at(opcode.kk())(*this, opcode);
}

void Dynarec::SKP_Vx(Opcode& opcode) {
    PC += system.frontend->keyIsPressed(Vx(opcode)) ? 4 : 2;
}

void Dynarec::SKNP_Vx(Opcode& opcode) {
    PC += system.frontend->keyIsPressed(Vx(opcode)) ? 2 : 4;
}

void Dynarec::split_F(Opcode& opcode) {
    opcode_table_F.at(opcode.kk())(*this, opcode);
}

void Dynarec::LD_Vx_DT(Opcode& opcode) {
    Vx(opcode) = DT;
    step();
}

void Dynarec::LD_Vx_K(Opcode& opcode) {
    Vx(opcode) = system.frontend->waitForInput();
    step();
}

void Dynarec::LD_DT_Vx(Opcode& opcode) {
    DT = Vx(opcode);
    step();
}

void Dynarec::LD_ST_Vx(Opcode& opcode) {
    ST = Vx(opcode);
    step();
}

void Dynarec::ADD_I_Vx(Opcode& opcode) {
    I += Vx(opcode);
    step();
}

void Dynarec::LD_F_Vx(Opcode& opcode) {
    I = Vx(opcode) * 5;
    step();
}

void Dynarec::LD_B_Vx(Opcode& opcode) {
    uint8_t num = Vx(opcode);
    system.memory[I] = num / 100;
    num %= 100;
    system.memory[I + 1] = num / 10;
    num %= 10;
    system.memory[I + 2] = num;
    step();
}

void Dynarec::LD_I_Vx(Opcode& opcode) {
    std::copy_n(std::execution::par_unseq, V.begin(), opcode.X() + 1, system.memory.begin() + I);
    step();
}

void Dynarec::LD_Vx_I(Opcode& opcode) {
    std::copy_n(std::execution::par_unseq, system.memory.begin() + I, opcode.X() + 1, V.begin());
    step();
}

Dynarec::OpcodeArray<0x10> Dynarec::opcode_table{
    &Dynarec::split_0,     &Dynarec::JP_addr,          &Dynarec::CALL_addr,  &Dynarec::SE_Vx_byte,
    &Dynarec::SNE_Vx_byte, &Dynarec::SE_Vx_Vy,         &Dynarec::LD_Vx_byte, &Dynarec::ADD_Vx_byte,
    &Dynarec::split_8,     &Dynarec::SNE_Vx_Vy,        &Dynarec::LD_I_addr,  &Dynarec::JP_0_addr,
    &Dynarec::RND_Vx_byte, &Dynarec::DRW_Vx_Vy_nibble, &Dynarec::split_E,    &Dynarec::split_F,
};

Dynarec::OpcodeMap Dynarec::opcode_table_0{
    {0xE0, &Dynarec::CLS},
    {0xEE, &Dynarec::RET},
};

Dynarec::OpcodeArray<0x10> Dynarec::opcode_table_8{
    &Dynarec::LD_Vx_Vy,
    &Dynarec::OR_Vx_Vy,
    &Dynarec::AND_Vx_Vy,
    &Dynarec::XOR_Vx_Vy,
    &Dynarec::ADD_Vx_Vy,
    &Dynarec::SUB_Vx_Vy,
    &Dynarec::SHR_Vx,
    &Dynarec::SUBN_Vx_Vy,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &Dynarec::SHL_Vx,
    nullptr,
};

Dynarec::OpcodeMap Dynarec::opcode_table_E{
    {0x9E, &Dynarec::SKP_Vx},
    {0xA1, &Dynarec::SKNP_Vx},
};

Dynarec::OpcodeMap Dynarec::opcode_table_F{
    {0x07, &Dynarec::LD_Vx_DT}, {0x0A, &Dynarec::LD_Vx_K},  {0x15, &Dynarec::LD_DT_Vx},
    {0x18, &Dynarec::LD_ST_Vx}, {0x1E, &Dynarec::ADD_I_Vx}, {0x29, &Dynarec::LD_F_Vx},
    {0x33, &Dynarec::LD_B_Vx},  {0x55, &Dynarec::LD_I_Vx},  {0x65, &Dynarec::LD_Vx_I},
};
