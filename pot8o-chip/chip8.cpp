#include <execution>
#include <fstream>

#include <istream>

//#include <vector>
#include "chip8.h"
#include "keypad.h"
#include "render.h"

using namespace std::chrono_literals;

Chip8::Chip8() {
    keypad = new Keypad();
    render = std::make_unique<Render>();
    cpu = std::make_unique<CPU>(this);

    rng.seed(std::random_device()());
    dist = std::make_unique<std::uniform_int_distribution<std::mt19937::result_type>>(0x00, 0xFF);

    initialize();
    loadGame("meep");
    emulate();
}

void Chip8::initialize() {
    pc = 0x200;
    opcode = 0;
    I = 0;
    sp = 0;

    std::fill(gfx.begin(), gfx.end(), 0);
    std::fill(stack.begin(), stack.end(), 0);
    std::fill(V.begin(), V.end(), 0);
    std::fill(memory.begin(), memory.end(), 0);

    std::copy(font.begin(), font.end(), memory.begin());

    delay_timer = 0;
    sound_timer = 0;
}

void Chip8::loadGame(std::string path) {
    path = "F:/git/chip8/TANK";
    std::ifstream file(path, std::ios::binary);
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(),
              memory.begin() + 0x200);
}

void Chip8::emulate() {
    while (true) {
        frame_start = std::chrono::steady_clock::now();
        emulateCycle();
        // std::this_thread::sleep_until(frame_start + 1ms);
    }
}

void Chip8::emulateCycle() {
    opcode = memory[pc] << 8 | memory[pc + 1];
    CPU::opcode_table[op()](*cpu.get());

    /*switch (opcode & 0xF000) {
    case 0x0000:
        switch (opcode & 0x00FF) {
        case 0x00E0:
            std::fill(gfx.begin(), gfx.end(), 0);
            break;

        case 0x00EE:
            pc = stack[sp];
            sp--;
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
        }
        break;

    case 0x1000:
        pc = (opcode & 0x0FFF) - 2;
        break;

    case 0x2000:
        sp++;
        stack[sp] = pc;
        pc = (opcode & 0x0FFF) - 2;
        break;

    case 0x3000:
        if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
            pc += 2;
        break;

    case 0x4000:
        if (V[(opcode & 0x0F00) >> 8] != (opcode & 0x00FF))
            pc += 2;
        break;

    case 0x6000:
        V[(opcode & 0x0F00) >> 8] = (opcode & 0x00FF);
        break;

    case 0x7000:
        V[(opcode & 0x0F00) >> 8] += (opcode & 0x00FF);
        break;

    case 0x8000:
        switch (opcode & 0x000F) {
        case 0x0000:
            V[(opcode & 0x0F00) >> 8] = V[(opcode & 0x00F0) >> 4];
            break;

        case 0x0001:
            V[(opcode & 0x0F00) >> 8] |= V[(opcode & 0x00F0) >> 4];

        case 0x0002:
            V[(opcode & 0x0F00) >> 8] &= V[(opcode & 0x00F0) >> 4];
            break;

        case 0x0003:
            V[(opcode & 0x0F00) >> 8] ^= V[(opcode & 0x00F0) >> 4];
            break;

        case 0x0004: {
            unsigned short result = V[(opcode & 0x0F00) >> 8] + V[(opcode & 0x00F0) >> 4];
            V[0xF] = result > 0xFF;
            V[(opcode & 0x0F00) >> 8] = static_cast<unsigned char>(result & 0xFF);
            break;
        }
        case 0x0005: {
            V[0xF] = V[(opcode & 0x0F00) >> 8] > V[(opcode & 0x00F0) >> 4];
            unsigned short result = V[(opcode & 0x0F00) >> 8] - V[(opcode & 0x00F0) >> 4];
            V[(opcode & 0x0F00) >> 8] = static_cast<unsigned char>(result & 0xFF);
            break;
        }
        case 0x0006:
            V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0b0000001) == 0b0000001;
            V[(opcode & 0x0F00) >> 8] >>= 1;
            break;

        case 0x000E:
            V[0xF] = (V[(opcode & 0x0F00) >> 8] & 0b1000000) == 0b1000000;
            V[(opcode & 0x0F00) >> 8] <<= 1;
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            break;
        }
        break;

    case 0x9000:
        if (V[(opcode & 0x0F00 >> 8)] != V[(opcode & 0x00F0 >> 4)])
            pc += 2;
        break;

    case 0xA000:
        I = opcode & 0x0FFF;
        break;
    case 0xC000: {
        V[(opcode & 0x0F00) >> 8] = rand() & (opcode & 0x00FF);
        break;
    }
    case 0xD000: {
        unsigned char x = V[(opcode & 0x0F00) >> 8] + 8;
        unsigned char height = opcode & 0x000F;
        unsigned char y = V[(opcode & 0x00F0) >> 4];
        V[0xF] = false;

        for (unsigned char row = 0; row < height; row++) {
            unsigned char byte = memory[I + row];
            for (char i = 0; i < 8; i++) {
                unsigned short& pixel = gfx[((y + row) % 32) * 64 + (x - i - 1) % 64];
                bool flip = byte & (1 << i);
                if (pixel && flip)
                    V[0xF] = true;
                if (flip)
                    pixel = ~pixel;
            }
        }

        render->drawGraphics(gfx);
        break;
    }
    case 0xE000:
        switch (opcode & 0x00FF) {
        case 0x009E:
            if (keypad->keyIsPressed((opcode & 0x0F00) >> 8))
                pc += 2;
            break;

        case 0x00A1:
            if (!keypad->keyIsPressed((opcode & 0x0F00) >> 8))
                pc += 2;
            break;
        }
        break;

    case 0xF000:
        switch (opcode & 0x00FF) {
        case 0x0007:
            V[(opcode & 0x0F00) >> 8] = delay_timer;
            break;

        case 0x000A:
            V[(opcode & 0x0F00) >> 8] = keypad->waitForInput();
            break;

        case 0x0015:
            delay_timer = V[(opcode & 0x0F00) >> 8];
            break;

        case 0x0018:
            sound_timer = V[(opcode & 0x0F00) >> 8];
            break;

        case 0x001E:
            I += V[(opcode & 0x0F00) >> 8];
            break;

        case 0x0029:
            I = V[(opcode & 0x0F00) >> 8] * 5;
            break;

        case 0x0033: {
            unsigned char num = V[(opcode & 0x0F00) >> 8];
            unsigned char cent = num / 100;
            num %= 100;
            unsigned char dec = num / 10;
            num %= 10;
            unsigned char unit = num;
            memory[I] = cent;
            memory[I + 1] = dec;
            memory[I + 2] = unit;
            break;
        }
        case 0x0055:
            std::copy_n(std::execution::par_unseq, V.begin(), ((opcode & 0x0F00) >> 8) + 1,
                        memory.begin() + I);
            break;

        case 0x0065:
            std::copy_n(std::execution::par_unseq, memory.begin() + I, ((opcode & 0x0F00) >> 8) + 1,
                        V.begin());
            break;

        default:
            printf("Unknown opcode [0x0000]: 0x%X\n", opcode);
            break;
        }
        break;

    default:
        printf("Unknown opcode: 0x%X\n", opcode);
        break;
    }*/

    if (delay_timer > 0)
        --delay_timer;

    if (sound_timer > 0) {
        if (sound_timer == 1)
            printf("BEEP!\n");
        --sound_timer;
    }
}

inline unsigned char Chip8::op() {
    return (opcode & 0xF000) >> 12;
}

inline unsigned char Chip8::X() {
    return (opcode & 0x0F00) >> 8;
}

inline unsigned char Chip8::Y() {
    return (opcode & 0x00F0) >> 4;
}

inline unsigned char& Chip8::Vx() {
    return V[X()];
}

inline unsigned char& Chip8::Vy() {
    return V[Y()];
}

inline unsigned char Chip8::n() {
    return opcode & 0x000F;
}

inline unsigned char Chip8::kk() {
    return opcode & 0x00FF;
}

inline unsigned short Chip8::nnn() {
    return opcode & 0x0FFF;
}

Chip8::CPU::CPU(Chip8* parent) : sys(parent) {}

void Chip8::CPU::split_0() {
    opcode_table_0[sys->kk()](*this);
}

void Chip8::CPU::CLS() {
    std::fill(sys->gfx.begin(), sys->gfx.end(), 0);
    sys->pc += 2;
}

void Chip8::CPU::RET() {
    sys->sp--;
    sys->pc = sys->stack[sys->sp] + 2;
}

void Chip8::CPU::JP_addr() {
    sys->pc = sys->nnn();
}

void Chip8::CPU::CALL_addr() {
    sys->stack[sys->sp] = sys->pc;
    sys->sp++;
    sys->pc = sys->nnn();
}

void Chip8::CPU::SE_Vx_byte() {
    sys->pc += sys->Vx() == sys->kk() ? 4 : 2;
}

void Chip8::CPU::SNE_Vx_byte() {
    sys->pc += sys->Vx() != sys->kk() ? 4 : 2;
}

void Chip8::CPU::SE_Vx_Vy() {
    sys->pc += sys->Vx() == sys->Vy() ? 4 : 2;
}

void Chip8::CPU::LD_Vx_byte() {
    sys->Vx() = sys->kk();
    sys->pc += 2;
}

void Chip8::CPU::ADD_Vx_byte() {
    sys->Vx() += sys->kk();
    sys->pc += 2;
}

void Chip8::CPU::split_8() {
    opcode_table_8[sys->n()](*this);
}

void Chip8::CPU::LD_Vx_Vy() {
    sys->Vx() = sys->Vy();
    sys->pc += 2;
}

void Chip8::CPU::OR_Vx_Vy() {
    sys->Vx() |= sys->Vy();
    sys->pc += 2;
}

void Chip8::CPU::AND_Vx_Vy() {
    sys->Vx() &= sys->Vy();
    sys->pc += 2;
}

void Chip8::CPU::XOR_Vx_Vy() {
    sys->Vx() ^= sys->Vy();
    sys->pc += 2;
}

void Chip8::CPU::ADD_Vx_Vy() {
    unsigned short result = sys->Vx() + sys->Vy();
    sys->V[0xF] = result > 0xFF;
    sys->Vx() = static_cast<unsigned char>(result & 0xFF);
    sys->pc += 2;
}

void Chip8::CPU::SUB_Vx_Vy() {
    sys->V[0xF] = sys->Vx() > sys->Vy();
    sys->Vx() -= sys->Vy();
    sys->pc += 2;
}

void Chip8::CPU::SHR_Vx() {
    sys->V[0xF] = sys->Vx() & 0b0000001;
    sys->Vx() >>= 1;
    sys->pc += 2;
}

void Chip8::CPU::SUBN_Vx_Vy() {
    sys->V[0xF] = sys->Vy() > sys->Vx();
    sys->Vx() = sys->Vy() - sys->Vx();
    sys->pc += 2;
}

void Chip8::CPU::SHL_Vx() {
    sys->V[0xF] = (sys->Vx() & 0b1000000) >> 7;
    sys->Vx() <<= 1;
    sys->pc += 2;
}

void Chip8::CPU::SNE_Vx_Vy() {
    sys->pc += sys->Vx() != sys->Vy() ? 4 : 2;
}

void Chip8::CPU::LD_I_addr() {
    sys->I = sys->nnn();
    sys->pc += 2;
}

void Chip8::CPU::JP_0_addr() {
    sys->pc = sys->nnn() + sys->V[0x0];
}

void Chip8::CPU::RND_Vx_byte() {
    sys->Vx() = sys->dist.get()->operator()(sys->rng) & sys->kk();
    sys->pc += 2;
}

void Chip8::CPU::DRW_Vx_Vy_nibble() {
    unsigned char x = sys->Vx() + 8;
    unsigned char height = sys->n();
    unsigned char y = sys->Vy();
    sys->V[0xF] = false;

    for (unsigned char row = 0; row < height; row++) {
        unsigned char byte = sys->memory[sys->I + row];
        for (char i = 0; i < 8; i++) {
            unsigned short& pixel = sys->gfx[((y + row) % 32) * 64 + (x - i - 1) % 64];
            bool flip = byte & (1 << i);
            if (pixel && flip)
                sys->V[0xF] = true;
            if (flip)
                pixel = ~pixel;
        }
    }

    sys->render->drawGraphics(sys->gfx);
    sys->pc += 2;
}

void Chip8::CPU::split_E() {
    opcode_table_E[sys->kk()](*this);
}

void Chip8::CPU::SKP_Vx() {
    sys->pc += sys->keypad->keyIsPressed(sys->Vx()) ? 4 : 2;
}

void Chip8::CPU::SKNP_Vx() {
    sys->pc += sys->keypad->keyIsPressed(sys->Vx()) ? 2 : 4;
}

void Chip8::CPU::split_F() {
    opcode_table_F[sys->kk()](*this);
}

void Chip8::CPU::LD_Vx_DT() {
    sys->Vx() = sys->delay_timer;
    sys->pc += 2;
}

void Chip8::CPU::LD_Vx_K() {
    sys->Vx() = sys->keypad->waitForInput();
    sys->pc += 2;
}

void Chip8::CPU::LD_DT_Vx() {
    sys->delay_timer = sys->Vx();
    sys->pc += 2;
}

void Chip8::CPU::LD_ST_Vx() {
    sys->sound_timer = sys->Vx();
    sys->pc += 2;
}

void Chip8::CPU::ADD_I_Vx() {
    sys->I += sys->Vx();
    sys->pc += 2;
}

void Chip8::CPU::LD_F_Vx() {
    sys->I = sys->Vx() * 5;
    sys->pc += 2;
}

void Chip8::CPU::LD_B_Vx() {
    unsigned char num = sys->Vx();
    sys->memory[sys->I] = num / 100;
    num %= 100;
    sys->memory[sys->I + 1] = num / 10;
    num %= 10;
    sys->memory[sys->I + 2] = num;
    sys->pc += 2;
}

void Chip8::CPU::LD_I_Vx() {
    std::copy_n(std::execution::par_unseq, sys->V.begin(), sys->X() + 1,
                sys->memory.begin() + sys->I);
    sys->pc += 2;
}

void Chip8::CPU::LD_Vx_I() {
    std::copy_n(std::execution::par_unseq, sys->memory.begin() + sys->I, sys->X() + 1,
                sys->V.begin());
    sys->pc += 2;
}

// clang-format off
const std::array<unsigned char, 0x50> Chip8::font{
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
