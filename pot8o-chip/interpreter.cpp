#include "interpreter.hpp"
#include "font.hpp"

void Interpreter::Run(Chip8::Interface& interface, std::vector<std::uint8_t> game) {
    this->interface = &interface;
	rng.seed(std::random_device()());

	std::copy(FONT.begin(), FONT.end(), memory.begin());
	std::copy(game.begin(), game.end(), memory.begin() + 0x200);

	for (;;) {
        if (program_counter >= 0x1000)
            return;
            opcode = memory[program_counter] << 8 | memory[program_counter + 1];
            (this->*opcode_table[op()])();
	}
}

void Interpreter::split_0() {
    (this->*opcode_table_0[kk()])();
}

void Interpreter::CLS() {
    std::fill(frame_buffer.begin(), frame_buffer.end(), 0);

	if (interface->GetSendFrame())
    interface->PushFrameBuffer(frame_buffer);
    step();
}

void Interpreter::RET() {
    program_counter = stack.back() + 2;
    stack.pop_back();
}

void Interpreter::JP_addr() {
    program_counter = nnn();
}

void Interpreter::CALL_addr() {
    stack.push_back(program_counter);
    program_counter = nnn();
}

void Interpreter::SE_Vx_byte() {
    program_counter += Vx() == kk() ? 4 : 2;
}

void Interpreter::SNE_Vx_byte() {
    program_counter += Vx() != kk() ? 4 : 2;
}

void Interpreter::SE_Vx_Vy() {
    program_counter += Vx() == Vy() ? 4 : 2;
}

void Interpreter::LD_Vx_byte() {
    Vx() = kk();
    step();
}

void Interpreter::ADD_Vx_byte() {
    Vx() += kk();
    step();
}

void Interpreter::split_8() {
    (this->*opcode_table_8[n()])();
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
    program_counter += Vx() != Vy() ? 4 : 2;
}

void Interpreter::LD_I_addr() {
    I = nnn();
    step();
}

void Interpreter::JP_V0_addr() {
    program_counter = nnn() + V[0x0];
}

void Interpreter::RND_Vx_byte() {
    Vx() = dist(rng) & kk();
    step();
}

void Interpreter::DRW_Vx_Vy_nibble() {
    const std::int32_t x = Vx() + std::uint8_t(8);
    const std::size_t y = Vy();
    const std::size_t height = n();
    std::uint64_t VF = 0;

    for (auto row = 0; row < height; ++row) {
        auto& fb_row = frame_buffer[(y + row) % 32];
        auto sprite_row = _rotr64(static_cast<std::uint64_t>(memory[I + row]), x);
        VF |= fb_row & sprite_row;
        fb_row ^= sprite_row;
    }
    V[0xF] = static_cast<bool>(VF);

	if (interface->GetSendFrame())
        interface->PushFrameBuffer(frame_buffer);

    step();
}

void Interpreter::split_E() {
    (this->*opcode_table_E[kk()])();
}

void Interpreter::SKP_Vx() {
    program_counter += interface->keypad_state[Vx()] ? 4 : 2;
}

void Interpreter::SKNP_Vx() {
    program_counter += interface->keypad_state[Vx()] ? 2 : 4;
}

void Interpreter::split_F() {
    (this->*opcode_table_F[kk()])();
}

void Interpreter::LD_Vx_DT() {
    Vx() = interface->delay_timer;
    step();
}

void Interpreter::LD_Vx_K() {
    Vx() = [this] {
        for (auto i = 0;; i = (i + 1) % interface->keypad_state.size())
            if (interface->keypad_state[i])
                return i;
	}();
    step();
}

void Interpreter::LD_DT_Vx() {
    interface->delay_timer = Vx();
    step();
}

void Interpreter::LD_ST_Vx() {
    interface->sound_timer = Vx();
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
    memory[I] = num / 100;
    num %= 100;
    memory[I + 1] = num / 10;
    num %= 10;
    memory[I + 2] = num;
    step();
}

void Interpreter::LD_I_Vx() {
    std::copy_n(V.begin(), X() + 1,
                memory.begin() + I);
    step();
}

void Interpreter::LD_Vx_I() {
    std::copy_n(memory.begin() + I, X() + 1,
                V.begin());
    step();
}