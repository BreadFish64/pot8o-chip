#pragma once
#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <random>
#include <string>

class Keypad;
class Render;

class Chip8 {
public:
    explicit Chip8();

    class CPU {
    public:
        explicit CPU(Chip8* parent);

        // Main jump table
        static const std::array<std::function<void(Chip8::CPU&)>, 0x10> opcode_table;

    private:
        Chip8* sys;

        // Jump table for opcodes starting with 0x0
        static const std::array<std::function<void(Chip8::CPU&)>, 0x100> opcode_table_0;
        // Jump table for opcodes starting with 0x8
        static const std::array<std::function<void(Chip8::CPU&)>, 0x10> opcode_table_8;
        // Jump table for opcodes starting with 0xE
        static const std::array<std::function<void(Chip8::CPU&)>, 0x100> opcode_table_E;
        // Jump table for opcodes starting with 0xF
        static const std::array<std::function<void(Chip8::CPU&)>, 0x100> opcode_table_F;

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

private:
    static const std::array<unsigned char, 80> font;

    Keypad* keypad = nullptr;
    std::unique_ptr<Render> render = nullptr;
    std::unique_ptr<CPU> cpu = nullptr;

    std::chrono::time_point<std::chrono::steady_clock> frame_start;

    std::mt19937 rng;
    std::unique_ptr<std::uniform_int_distribution<std::mt19937::result_type>> dist;

    std::array<unsigned short, 64 * 32> gfx;
    std::array<unsigned short, 16> stack;
    std::array<unsigned char, 0x1000> memory;
    std::array<unsigned char, 16> V;
    unsigned short opcode;
    unsigned short I;
    unsigned short pc;
    unsigned short sp;
    unsigned char delay_timer;
    unsigned char sound_timer;

    void initialize();
    void emulate();
    void emulateCycle();
    void loadGame(std::string path);

    inline unsigned char op();
    inline unsigned char X();
    inline unsigned char Y();
    inline unsigned char& Vx();
    inline unsigned char& Vy();
    inline unsigned char n();
    inline unsigned char kk();
    inline unsigned short nnn();
};
