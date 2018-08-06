#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>
#include "chip8.h"

class Interpreter : public CPU {
public:
    explicit Interpreter(Chip8* parent);
    ~Interpreter();

    void execute() override;

private:
    Chip8& system;

    std::chrono::time_point<std::chrono::steady_clock> cycle_start;

    // Used for random number generation in intruction 0xC
    std::mt19937 rng;
    std::uniform_int_distribution<std::mt19937::result_type> dist =
        std::uniform_int_distribution<std::mt19937::result_type>(0x00, 0xFF);

    // system registers
    std::array<uint8_t, 0x10> V;
    // contains addresses to return from calls
    std::vector<uint_fast16_t> stack;
    // contains a single memory address
    uint16_t I;
    // counts down each cycle
    uint8_t DT;
    // counts down each cycle, beeps when 0 is hit
    uint8_t ST;
    // location in memory corresponding to the current instruction
    uint_fast16_t PC;

    Opcode opcode;

    // Main jump table
    static const std::array<std::function<void(Interpreter&)>, 0x10> opcode_table;
    // Jump table for opcodes starting with 0x0
    static const std::array<std::function<void(Interpreter&)>, 0x100> opcode_table_0;
    // Jump table for opcodes starting with 0x8
    static const std::array<std::function<void(Interpreter&)>, 0x10> opcode_table_8;
    // Jump table for opcodes starting with 0xE
    static const std::array<std::function<void(Interpreter&)>, 0x100> opcode_table_E;
    // Jump table for opcodes starting with 0xF
    static const std::array<std::function<void(Interpreter&)>, 0x100> opcode_table_F;

    // Increase PC by 2
    inline void step();

    // returns reference to register X
    inline uint8_t& Vx();
    // returns reference to register Y
    inline uint8_t& Vy();

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
    void JP_0_addr();
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
};
