#include <algorithm>
#include <array>
#include <cstdint>
#include <random>
#include <vector>
#include <sstream>

#include "chip8.hpp"

class LLVMAOT final : public Chip8::CPU {
public:
    void Run(Chip8::Interface& interface, std::vector<std::uint8_t> game) override;

private:

	void NOOP();
    // Call sub-table for opcodes starting with 0x0
    void split_0();
    // Clear the display
    void CLS();
    // Return from a subroutine
    void RET();

    // Jump to location nnn
    void JP_addr();
    // Call subroutine at nnn
    void CALL_addr();
    // Skip next instruction if V[x] == kk
    void SE_Vx_byte();
    // Skip next instruction if V[x] != kk
    void SNE_Vx_byte();
    // Skip next instruction if V[x] == V[y]
    void SE_Vx_Vy();
    // Set V[x] to kk
    void LD_Vx_byte();
    // Set V[x] to V[x] + kk
    void ADD_Vx_byte();

    // Call sub-table for opcodes starting with 0x8
    void split_8();
    // Set V[x] to V[y]
    void LD_Vx_Vy();
    // Set V[x] to V[x] | V[y]
    void OR_Vx_Vy();
    // Set V[x] to V[x] & V[y]
    void AND_Vx_Vy();
    // Set V[x] to V[x] ^ V[y]
    void XOR_Vx_Vy();
    // Set V[x] to V[x] + V[y]
    void ADD_Vx_Vy();
    // Set V[x] to V[x] - V[y]
    void SUB_Vx_Vy();
    // Set V[x] to V[x] >> V[y]
    void SHR_Vx();
    // Set V[x] to V[y] - V[x]
    void SUBN_Vx_Vy();
    // Set V[x] to V[x] << V[y]
    void SHL_Vx();

    // Skip next instruction if V[x] != V[y]
    void SNE_Vx_Vy();
    // Set I to nnn
    void LD_I_addr();
    // Jump to nnn + V[0]
    void JP_V0_addr();
    // V[x] = random byte & kk
    void RND_Vx_byte();
    // Display n-byte sprite starting at memory location I at (V[x], V[y]), set VF = collision
    void DRW_Vx_Vy_nibble();

    // Call sub-table for opcodes starting with 0xE
    void split_E();
    // Skip next instruction if key with the value of V[x] is pressed
    void SKP_Vx();
    // Skip next instruction if key with the value of V[x] is not pressed
    void SKNP_Vx();

    // Call sub-table for opcodes starting with 0xF
    void split_F();
    // Set V[x] to DT
    void LD_Vx_DT();
    // Wait for a key press, store the value of the key in V[x]
    void LD_Vx_K();
    // Set DT to V[x]
    void LD_DT_Vx();
    // Set ST to V[x]
    void LD_ST_Vx();
    // Set I to I + V[x]
    void ADD_I_Vx();
    // Set I to address of sprite for digit Vx
    void LD_F_Vx();
    // Store BCD representation of V[x] in memory locations I, I+1, and I+2
    void LD_B_Vx();
    // Store registers V[0] through V[x] in memory starting at location I
    void LD_I_Vx();
    // Read registers V[0] through V[x] from memory starting at location I
    void LD_Vx_I();

    inline std::uint8_t op() {
        return (opcode & 0xF000) >> 12;
    }

    inline std::uint8_t X() {
        return (opcode & 0x0F00) >> 8;
    }

    inline std::uint8_t Y() {
        return (opcode & 0x00F0) >> 4;
    }

    inline std::uint8_t n() {
        return opcode & 0x000F;
    }

    inline std::uint8_t kk() {
        return opcode & 0x00FF;
    }

    inline std::size_t nnn() {
        return opcode & 0x0FFF;
    }

    // current instruction
    std::uint16_t opcode = 0;
    // location in memory corresponding to the current instruction
    std::size_t program_counter = 0x200;

	std::stringstream source_builder;

    using Instruction = decltype(&LLVMAOT::NOOP);

    // clang-format off
	static constexpr std::array<Instruction, 0x10> opcode_table{
		&LLVMAOT::split_0,		&LLVMAOT::JP_addr,			&LLVMAOT::CALL_addr,	&LLVMAOT::SE_Vx_byte,
		&LLVMAOT::SNE_Vx_byte,	&LLVMAOT::SE_Vx_Vy,			&LLVMAOT::LD_Vx_byte,	&LLVMAOT::ADD_Vx_byte,
		&LLVMAOT::split_8,		&LLVMAOT::SNE_Vx_Vy,		&LLVMAOT::LD_I_addr,	&LLVMAOT::JP_V0_addr,
		&LLVMAOT::RND_Vx_byte,	&LLVMAOT::DRW_Vx_Vy_nibble,	&LLVMAOT::split_E,		&LLVMAOT::split_F
	};

	static constexpr std::array<Instruction, 0x100> opcode_table_0 = []{
		std::array<Instruction, 0x100> table{};
		for (auto& op : table) op = &LLVMAOT::NOOP;
		table[0xE0] = &LLVMAOT::CLS;
		table[0xEE] = &LLVMAOT::RET;
		return table;
	}();

	static constexpr std::array<Instruction, 0x10> opcode_table_8 {
		&LLVMAOT::LD_Vx_Vy,		&LLVMAOT::OR_Vx_Vy,		&LLVMAOT::AND_Vx_Vy,	&LLVMAOT::XOR_Vx_Vy,
		&LLVMAOT::ADD_Vx_Vy,	&LLVMAOT::SUB_Vx_Vy,	&LLVMAOT::SHR_Vx,		&LLVMAOT::SUBN_Vx_Vy,
		&LLVMAOT::NOOP,			&LLVMAOT::NOOP,			&LLVMAOT::NOOP,			&LLVMAOT::NOOP,
		&LLVMAOT::NOOP,			&LLVMAOT::NOOP,			&LLVMAOT::SHL_Vx,		&LLVMAOT::NOOP
	};

	static constexpr std::array<Instruction, 0x100> opcode_table_E = []{
		std::array<Instruction, 0x100> table{};
		for (auto& op : table) op = &LLVMAOT::NOOP;
		table[0x9E] = &LLVMAOT::SKP_Vx;
		table[0xA1] = &LLVMAOT::SKNP_Vx;
		return table;
	}();

	static constexpr std::array<Instruction, 0x100> opcode_table_F = []{
		std::array<Instruction, 0x100> table{};
		for (auto& op : table) op = &LLVMAOT::NOOP;
		table[0x07] = &LLVMAOT::LD_Vx_DT;
		table[0x0A] = &LLVMAOT::LD_Vx_K;
		table[0x15] = &LLVMAOT::LD_DT_Vx;
		table[0x18] = &LLVMAOT::LD_ST_Vx;
		table[0x1E] = &LLVMAOT::ADD_I_Vx;
		table[0x29] = &LLVMAOT::LD_F_Vx;
		table[0x33] = &LLVMAOT::LD_B_Vx;
		table[0x55] = &LLVMAOT::LD_I_Vx;
		table[0x65] = &LLVMAOT::LD_Vx_I;
		return table;
	}();
    // clang-format on
};