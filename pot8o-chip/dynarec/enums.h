#pragma once
namespace GuestRegs {
static constexpr int V0 = 0;
static constexpr int V1 = 1;
static constexpr int V2 = 2;
static constexpr int V3 = 3;
static constexpr int V4 = 4;
static constexpr int V5 = 5;
static constexpr int V6 = 6;
static constexpr int V7 = 7;
static constexpr int V8 = 8;
static constexpr int V9 = 9;
static constexpr int VA = 0xA;
static constexpr int VB = 0xB;
static constexpr int VC = 0xC;
static constexpr int VD = 0xD;
static constexpr int VE = 0xE;
static constexpr int VF = 0xF;
static constexpr int DT = 16;
static constexpr int ST = 17;
static constexpr int I = 18;
}; // namespace GuestRegs

namespace Instructions {
// Clear the display
static constexpr uint8_t CLS = 0;
// Return from a subroutine
static constexpr uint8_t RET = 1;

// Jump to location nnn
static constexpr uint8_t JP_addr = 2;
// Call subroutine at nnn
static constexpr uint8_t CALL_addr = 3;
// Skip next instruction if V[x] == kk
static constexpr uint8_t SE_Vx_byte = 4;
// Skip next instruction if V[x] != kk
static constexpr uint8_t SNE_Vx_byte = 5;
// Skip next instruction if V[x] == V[y]
static constexpr uint8_t SE_Vx_Vy = 6;
// Set V[x] to kk
static constexpr uint8_t LD_Vx_byte = 7;
// Set V[x] to V[x] + kk
static constexpr uint8_t ADD_Vx_byte = 8;

// Set V[x] to V[y]
static constexpr uint8_t LD_Vx_Vy = 9;
// Set V[x] to V[x] | V[y]
static constexpr uint8_t OR_Vx_Vy = 10;
// Set V[x] to V[x] & V[y]
static constexpr uint8_t AND_Vx_Vy = 11;
// Set V[x] to V[x] ^ V[y]
static constexpr uint8_t XOR_Vx_Vy = 12;
// Set V[x] to V[x] + V[y]
static constexpr uint8_t ADD_Vx_Vy = 13;
// Set V[x] to V[x] - V[y]
static constexpr uint8_t SUB_Vx_Vy = 14;
// Set V[x] to V[x] >> V[y]
static constexpr uint8_t SHR_Vx = 15;
// Set V[x] to V[y] - V[x]
static constexpr uint8_t SUBN_Vx_Vy = 16;
// Set V[x] to V[x] << V[y]
static constexpr uint8_t SHL_Vx = 17;

// Skip next instruction if V[x] != V[y]
static constexpr uint8_t SNE_Vx_Vy = 18;
// Set I to nnn
static constexpr uint8_t LD_I_addr = 19;
// Jump to nnn + V[0]
static constexpr uint8_t JP_V0_addr = 20;
// V[x] = random byte & kk
static constexpr uint8_t RND_Vx_byte = 21;
// Display n-byte sprite starting at memory location I at (V[x], V[y]), set VF = collision
static constexpr uint8_t DRW_Vx_Vy_nibble = 22;

// Skip next instruction if key with the value of V[x] is pressed
static constexpr uint8_t SKP_Vx = 23;
// Skip next instruction if key with the value of V[x] is not pressed
static constexpr uint8_t SKNP_Vx = 24;

// Set V[x] to DT
static constexpr uint8_t LD_Vx_DT = 25;
// Wait for a key press, store the value of the key in V[x]
static constexpr uint8_t LD_Vx_K = 26;
// Set DT to V[x]
static constexpr uint8_t LD_DT_Vx = 27;
// Set ST to V[x]
static constexpr uint8_t LD_ST_Vx = 28;
// Set I to I + V[x]
static constexpr uint8_t ADD_I_Vx = 29;
// Set I to address of sprite for digit Vx
static constexpr uint8_t LD_F_Vx = 30;
// Store BCD representation of V[x] in memory locations I, I+1, and I+2
static constexpr uint8_t LD_B_Vx = 31;
// Store registers V[0] through V[x] in memory starting at location I
static constexpr uint8_t LD_I_Vx = 32;
// Read registers V[0] through V[x] from memory starting at location I
static constexpr uint8_t LD_Vx_I = 33;

static constexpr uint8_t INVALID_OPCODE = 255;
}; // namespace Instructions
