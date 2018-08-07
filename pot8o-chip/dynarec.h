#pragma once

#include <array>
#include <functional>
#include <unordered_map>

#include "chip8.h"

class Dynarec : public CPU {
public:
    explicit Dynarec(Chip8* parent);
    ~Dynarec();

    void execute() override;

private:
    Chip8& system;

    using InstructionList = std::vector<std::function<void(Dynarec&)>>;

    std::unordered_map<uint16_t, InstructionList> code_cache;

    // system registers
    std::array<uint8_t, 0x10> V;
    // counts down each cycle
    uint8_t DT;
    // counts down each cycle, beeps when 0 is hit
    uint8_t ST;
    // location in memory corresponding to the current instruction
    uint16_t PC;
    // contains a single memory adress
    uint16_t I;
    // contains addresses to return from calls
    std::vector<uint_fast16_t> stack;

    //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
    template <size_t size>
    using OpcodeArray = const std::array<std::function<void(Dynarec&, Opcode&)>, size>;
    using OpcodeMap = const std::unordered_map<uint16_t, std::function<void(Dynarec&, Opcode&)>>;
    // Main jump table
    static OpcodeArray<0x10> opcode_table;
    // Jump table for opcodes starting with 0x0
    static OpcodeMap opcode_table_0;
    // Jump table for opcodes starting with 0x8
    static OpcodeArray<0x10> opcode_table_8;
    // Jump table for opcodes starting with 0xE
    static OpcodeMap opcode_table_E;
    // Jump table for opcodes starting with 0xF
    static OpcodeMap opcode_table_F;

    // Increase PC by 2
    inline void step();

    inline void timerStep();

    bool isBranchInstruction(Opcode& opcode);

    // returns reference to register X
    inline uint8_t& Vx(Opcode& opcode);
    // returns reference to register Y
    inline uint8_t& Vy(Opcode& opcode);

    // Call sub-table for opcodes starting with 0x0
    void split_0(Opcode& opcode);
    // Clear the display
    void CLS(Opcode& opcode);
    // Return from a subroutine
    void RET(Opcode& opcode);

    // Jump to location nnn
    void JP_addr(Opcode& opcode);
    // Call subroutine at nnn
    void CALL_addr(Opcode& opcode);
    // Skip next instruction if V[x] == kk
    void SE_Vx_byte(Opcode& opcode);
    // Skip next instruction if V[x] != kk
    void SNE_Vx_byte(Opcode& opcode);
    // Skip next instruction if V[x] == V[y]
    void SE_Vx_Vy(Opcode& opcode);
    // Set V[x] to kk
    void LD_Vx_byte(Opcode& opcode);
    // Set V[x] to V[x] + kk
    void ADD_Vx_byte(Opcode& opcode);

    // Call sub-table for opcodes starting with 0x8
    void split_8(Opcode& opcode);
    // Set V[x] to V[y]
    void LD_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] | V[y]
    void OR_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] & V[y]
    void AND_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] ^ V[y]
    void XOR_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] + V[y]
    void ADD_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] - V[y]
    void SUB_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] >> V[y]
    void SHR_Vx(Opcode& opcode);
    // Set V[x] to V[y] - V[x]
    void SUBN_Vx_Vy(Opcode& opcode);
    // Set V[x] to V[x] << V[y]
    void SHL_Vx(Opcode& opcode);

    // Skip next instruction if V[x] != V[y]
    void SNE_Vx_Vy(Opcode& opcode);
    // Set I to nnn
    void LD_I_addr(Opcode& opcode);
    // Jump to nnn + V[0]
    void JP_0_addr(Opcode& opcode);
    // V[x] = random byte & kk
    void RND_Vx_byte(Opcode& opcode);
    // Display n-byte sprite starting at memory location I at (V[x], V[y]), set VF = collision
    void DRW_Vx_Vy_nibble(Opcode& opcode);

    // Call sub-table for opcodes starting with 0xE
    void split_E(Opcode& opcode);
    // Skip next instruction if key with the value of V[x] is pressed
    void SKP_Vx(Opcode& opcode);
    // Skip next instruction if key with the value of V[x] is not pressed
    void SKNP_Vx(Opcode& opcode);

    // Call sub-table for opcodes starting with 0xF
    void split_F(Opcode& opcode);
    // Set V[x] to DT
    void LD_Vx_DT(Opcode& opcode);
    // Wait for a key press, store the value of the key in V[x]
    void LD_Vx_K(Opcode& opcode);
    // Set DT to V[x]
    void LD_DT_Vx(Opcode& opcode);
    // Set ST to V[x]
    void LD_ST_Vx(Opcode& opcode);
    // Set I to I + V[x]
    void ADD_I_Vx(Opcode& opcode);
    // Set I to address of sprite for digit Vx
    void LD_F_Vx(Opcode& opcode);
    // Store BCD representation of V[x] in memory locations I, I+1, and I+2
    void LD_B_Vx(Opcode& opcode);
    // Store registers V[0] through V[x] in memory starting at location I
    void LD_I_Vx(Opcode& opcode);
    // Read registers V[0] through V[x] from memory starting at location I
    void LD_Vx_I(Opcode& opcode);
    //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
};
