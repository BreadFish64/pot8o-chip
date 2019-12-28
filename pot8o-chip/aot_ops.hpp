constexpr char AOT_OPS[] = R"(

using u8 = unsigned char;
using u64 = unsigned long long;
using Frame = u64[32];

constexpr u8 FONT[]{
    // 0
    0b11110000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b11110000,
    // 1
    0b00100000,
    0b01100000,
    0b00100000,
    0b00100000,
    0b01110000,
    // 2
    0b11110000,
    0b00010000,
    0b11110000,
    0b10000000,
    0b11110000,
    // 3
    0b11110000,
    0b00010000,
    0b11110000,
    0b00010000,
    0b11110000,
    // 4
    0b10010000,
    0b10010000,
    0b11110000,
    0b00010000,
    0b00010000,
    // 5
    0b11110000,
    0b10000000,
    0b11110000,
    0b00010000,
    0b11110000,
    // 6
    0b11110000,
    0b10000000,
    0b11110000,
    0b10010000,
    0b11110000,
    // 7
    0b11110000,
    0b00010000,
    0b00100000,
    0b01000000,
    0b01000000,
    // 8
    0b11110000,
    0b10010000,
    0b11110000,
    0b10010000,
    0b11110000,
    // 9
    0b11110000,
    0b10010000,
    0b11110000,
    0b00010000,
    0b11110000,
    // A
    0b11110000,
    0b10010000,
    0b11110000,
    0b10010000,
    0b10010000,
    // B
    0b11100000,
    0b10010000,
    0b11100000,
    0b10010000,
    0b11100000,
    // C
    0b11110000,
    0b10000000,
    0b10000000,
    0b10000000,
    0b11110000,
    // D
    0b11100000,
    0b10010000,
    0b10010000,
    0b10010000,
    0b11100000,
    // E
    0b11110000,
    0b10000000,
    0b11110000,
    0b10000000,
    0b11110000,
    // F
    0b11110000,
    0b10000000,
    0b11110000,
    0b10000000,
    0b10000000,
};

template <typename T>
class Atomic {
    T val;

public:
    inline operator T() const {
        return __atomic_load_n(&val, __ATOMIC_RELAXED);
    }
    inline void operator=(const T& rhs) {
        __atomic_store_n(&this->val, rhs, __ATOMIC_RELAXED);
    }
    inline T operator++(int) {
        return __atomic_fetch_add(&val, 1, __ATOMIC_RELAXED);
    }
    inline T operator+=(const T& rhs) {
        return __atomic_fetch_add(&val, rhs, __ATOMIC_RELAXED);
    }
};

class Interface {
    Frame frame_buffer;
public:
    Atomic<bool> keypad_state[16];
    // actually twice the cycle count but the frontend can figure out that calculation
    Atomic<u64> cycle_count;
    Atomic<u8> delay_timer;
    Atomic<u8> sound_timer;
private:
    Atomic<bool> send_frame;
    Atomic<bool> stop_flag;

public:
    void PushFrame(Frame& frame) {
        // TODO: figure out how to do this efficiently without missing frames at the end of the
        // program
        if (send_frame) {
            __builtin_memcpy(&frame_buffer, &frame, sizeof(frame));
            send_frame = false;
        }
        if (stop_flag);
    }
};

static u8 memory[0x1000];
static u64 frame_buffer[32]{};
static u8 V[16]{};
static void* stack[16]{};
static unsigned stack_ptr{};
static unsigned I{};
static unsigned rand = seed;
static unsigned last_jump;

namespace Opcodes {
void CLS() {
    for (auto& row : frame_buffer)
        row = 0;
    interface.PushFrame(frame_buffer);
}

#define RET(pc)                                                                                    \
    interface.cycle_count += pc - last_jump;                                                       \
    goto* stack[--stack_ptr];

#define JP_addr(pc, addr)                                                                          \
    interface.cycle_count += pc - last_jump;                                                       \
    last_jump = addr;                                                                              \
    goto* jump_table[addr];

#define CALL_addr(pc, addr)                                                                        \
    interface.cycle_count += pc - last_jump;                                                       \
    last_jump = addr;                                                                              \
    stack[stack_ptr++] = &&l##pc##_ret;                                                            \
    goto* jump_table[addr];                                                                        \
    l##pc##_ret:                                                                                   \
    last_jump = pc + 2;                                                                            \

#define SE_Vx_byte(pc, x, byte)                                                                    \
    if (V[x] == byte)                                                                              \
        goto* jump_table[pc + 4];

#define SNE_Vx_byte(pc, x, byte)                                                                   \
    if (V[x] != byte)                                                                              \
        goto* jump_table[pc + 4];

#define SE_Vx_Vy(pc, x, y)                                                                         \
    if (V[x] == V[y])                                                                              \
        goto* jump_table[pc + 4];

template <unsigned x, u8 byte>
void LD_Vx_byte() {
    V[x] = byte;
}

template <unsigned x, u8 byte>
void ADD_Vx_byte() {
    V[x] += byte;
}

template <unsigned x, unsigned y>
void LD_Vx_Vy() {
    V[x] = V[y];
}

template <unsigned x, unsigned y>
void OR_Vx_Vy() {
    V[x] |= V[y];
}

template <unsigned x, unsigned y>
void AND_Vx_Vy() {
    V[x] &= V[y];
}

template <unsigned x, unsigned y>
void XOR_Vx_Vy() {
    V[x] ^= V[y];
}

template <unsigned x, unsigned y>
void ADD_Vx_Vy() {
    V[x] = __builtin_addcb(V[x], V[y], 0, &V[0xF]);
}

template <unsigned x, unsigned y>
void SUB_Vx_Vy() {
    u8 flag;
    V[x] = __builtin_subcb(V[x], V[y], 0, &flag);
    V[0xF] = !flag;
}

template <unsigned x>
void SHR_Vx() {
    V[0xF] = V[x] & 0b0000001;
    V[x] >>= 1;
}

template <unsigned x, unsigned y>
void SUBN_Vx_Vy() {
    u8 flag;
    V[x] = __builtin_subcb(V[y], V[x], 0, &flag);
    V[0xF] = !flag;
}

template <unsigned x>
void SHL_Vx() {
    V[0xF] = V[x] >> 7;
    V[x] <<= 1;
}

#define SNE_Vx_Vy(pc, x, y)                                                                         \
    if (V[x] != V[y])                                                                              \
        goto* jump_table[pc + 4];

template <unsigned addr>
void LD_I_addr() {
    I = addr;
}

#define JP_V0_addr(pc, addr)                                                                       \
    interface.cycle_count += pc - last_jump;                                                       \
    last_jump = addr + V[0x0];                                                                     \
    goto* jump_table[addr + V[0x0]];

template <unsigned x, u8 byte>
void RND_Vx_byte() {
    rand ^= rand << 13;
    rand ^= rand >> 17;
    rand ^= rand << 5;
    V[x] = rand & byte;
}

template <unsigned x, unsigned y, unsigned height>
void DRW_Vx_Vy_nibble() {
    const auto left = V[x] + 7;
    const auto top = V[y];
    u64 flag = 0;

    for (auto row = 0; row < height; row++) {
        auto& fb_row = frame_buffer[(top + row) % 32];
        auto sprite_row = __builtin_rotateright64(memory[I + row], left);
        flag |= fb_row & sprite_row;
        fb_row ^= sprite_row;
    }
    V[0xF] = static_cast<bool>(flag);

    interface.PushFrame(frame_buffer);
}

#define SKP_Vx(pc, x)                                                                              \
    if (interface.keypad_state[V[x]])                                                              \
        goto* jump_table[pc + 4];

#define SKNP_Vx(pc, x)                                                                             \
    if (!interface.keypad_state[V[x]])                                                             \
        goto* jump_table[pc + 4];

template <unsigned x>
void LD_Vx_DT() {
    V[x] = interface.delay_timer;
}

template <unsigned x>
void LD_Vx_K() {
    V[x] = [] {
        for (auto i = 0;; i = (i + 1) % (sizeof(interface.keypad_state) / sizeof(Atomic<bool>)))
            if (interface.keypad_state[i])
                return i;
    }();
}

template <unsigned x>
void LD_DT_Vx() {
    interface.delay_timer = V[x];
}

template <unsigned x>
void LD_ST_Vx() {
    interface.sound_timer = V[x];
}

template <unsigned x>
void ADD_I_Vx() {
    I += V[x];
}

template <unsigned x>
void LD_F_Vx() {
    I = V[x] * 5;
}

template <unsigned x>
void LD_B_Vx() {
    auto num = V[x];
    memory[I] = num / 100;
    num %= 100;
    memory[I + 1] = num / 10;
    num %= 10;
    memory[I + 2] = num;
}

template <unsigned x>
void LD_I_Vx() {
    for (auto i = 0; i <= x; i++)
        memory[I + i] = V[i];
}

template <unsigned x>
void LD_Vx_I() {
    for (auto i = 0; i <= x; i++)
        V[i] = memory[I + i];
}
} // namespace Opcodes
)";
