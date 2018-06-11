#include <execution>
#include <fstream>
#include <iostream>
#include <istream>
#include "chip8.h"

Chip8::Chip8() {
    renderer = std::make_unique<Renderer>();
    keypad = std::make_unique<Keypad>(this, renderer.get());
    dist = std::make_unique<std::uniform_int_distribution<std::mt19937::result_type>>(0x00, 0xFF);
    frame_length = std::chrono::duration<double, std::milli>(1000.0 / target_clock_speed);
}

void Chip8::changeSpeed(signed int diff) {
    target_clock_speed = (target_clock_speed + diff > 0) ? (target_clock_speed + diff) : 1;
    frame_length = std::chrono::duration<double, std::milli>(1000.0 / target_clock_speed);
    renderer->setTitleBar(title + " - " + std::to_string(target_clock_speed) + "hz");
}

void Chip8::initialize() {
    rng.seed(std::random_device()());

    pc = 0x200;
    opcode = 0;
    I = 0;

    std::fill(gfx.begin(), gfx.end(), 0);
    std::fill(V.begin(), V.end(), 0);
    std::fill(memory.begin(), memory.end(), 0);

    stack.clear();

    std::copy(font.begin(), font.end(), memory.begin());

    delay_timer = 0;
    sound_timer = 0;
}

void Chip8::loadGame(std::string path) {
    initialize();
    if (path.at(path.size() - 1) == '"')
        path.erase(path.size() - 1);
    if (path.at(0) == '"')
        path.erase(0, 1);
    title = path.substr(path.find_last_of('/') + 1, path.size() - path.find_last_of('/') - 1);
    renderer->setTitleBar(title + " - " + std::to_string(target_clock_speed) + "hz");
    std::ifstream file(path, std::ios::binary);
    std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(),
              memory.begin() + 0x200);
}

void Chip8::emulate() {
    frame_start = std::chrono::steady_clock::now();
    while (true) {
        emulateCycle();
        keypad->checkInput();
        if (limitSpeed)
            std::this_thread::sleep_until(
                frame_start +=
                std::chrono::duration_cast<std::chrono::steady_clock::duration>(frame_length));
    }
}

void Chip8::emulateCycle() {
    opcode = memory[pc] << 8 | memory[pc + 1];
    CPU::opcode_table[op()](cpu);

    if (delay_timer > 0)
        --delay_timer;

    if (sound_timer > 0) {
        if (sound_timer == 1)
            std::cout << "BEEP" << std::endl;
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

Chip8::CPU::CPU(Chip8* parent) : sys(*parent) {}

void Chip8::CPU::split_0() {
    opcode_table_0[sys.kk()](*this);
}

void Chip8::CPU::CLS() {
    std::fill(std::execution::par_unseq, sys.gfx.begin(), sys.gfx.end(), 0);
    sys.pc += 2;
}

void Chip8::CPU::RET() {
    sys.pc = sys.stack.back() + 2;
    sys.stack.pop_back();
}

void Chip8::CPU::JP_addr() {
    sys.pc = sys.nnn();
}

void Chip8::CPU::CALL_addr() {
    sys.stack.push_back(sys.pc);
    sys.pc = sys.nnn();
}

void Chip8::CPU::SE_Vx_byte() {
    sys.pc += sys.Vx() == sys.kk() ? 4 : 2;
}

void Chip8::CPU::SNE_Vx_byte() {
    sys.pc += sys.Vx() != sys.kk() ? 4 : 2;
}

void Chip8::CPU::SE_Vx_Vy() {
    sys.pc += sys.Vx() == sys.Vy() ? 4 : 2;
}

void Chip8::CPU::LD_Vx_byte() {
    sys.Vx() = sys.kk();
    sys.pc += 2;
}

void Chip8::CPU::ADD_Vx_byte() {
    sys.Vx() += sys.kk();
    sys.pc += 2;
}

void Chip8::CPU::split_8() {
    opcode_table_8[sys.n()](*this);
}

void Chip8::CPU::LD_Vx_Vy() {
    sys.Vx() = sys.Vy();
    sys.pc += 2;
}

void Chip8::CPU::OR_Vx_Vy() {
    sys.Vx() |= sys.Vy();
    sys.pc += 2;
}

void Chip8::CPU::AND_Vx_Vy() {
    sys.Vx() &= sys.Vy();
    sys.pc += 2;
}

void Chip8::CPU::XOR_Vx_Vy() {
    sys.Vx() ^= sys.Vy();
    sys.pc += 2;
}

void Chip8::CPU::ADD_Vx_Vy() {
    unsigned short result = sys.Vx() + sys.Vy();
    sys.V[0xF] = result > 0xFF;
    sys.Vx() = static_cast<unsigned char>(result & 0xFF);
    sys.pc += 2;
}

void Chip8::CPU::SUB_Vx_Vy() {
    sys.V[0xF] = sys.Vx() > sys.Vy();
    sys.Vx() -= sys.Vy();
    sys.pc += 2;
}

void Chip8::CPU::SHR_Vx() {
    sys.V[0xF] = sys.Vx() & 0b0000001;
    sys.Vx() >>= 1;
    sys.pc += 2;
}

void Chip8::CPU::SUBN_Vx_Vy() {
    sys.V[0xF] = sys.Vy() > sys.Vx();
    sys.Vx() = sys.Vy() - sys.Vx();
    sys.pc += 2;
}

void Chip8::CPU::SHL_Vx() {
    sys.V[0xF] = (sys.Vx() & 0b1000000) >> 7;
    sys.Vx() <<= 1;
    sys.pc += 2;
}

void Chip8::CPU::SNE_Vx_Vy() {
    sys.pc += sys.Vx() != sys.Vy() ? 4 : 2;
}

void Chip8::CPU::LD_I_addr() {
    sys.I = sys.nnn();
    sys.pc += 2;
}

void Chip8::CPU::JP_0_addr() {
    sys.pc = sys.nnn() + sys.V[0x0];
}

void Chip8::CPU::RND_Vx_byte() {
    sys.Vx() = sys.dist->operator()(sys.rng) & sys.kk();
    sys.pc += 2;
}

void Chip8::CPU::DRW_Vx_Vy_nibble() {
    unsigned char x = sys.Vx() + 71;
    unsigned char y = sys.Vy();
    unsigned char height = sys.n();
    sys.V[0xF] = false;

    for (unsigned char row = 0; row < height; row++) {
        unsigned char byte = sys.memory[sys.I + row];
        for (char i = 0; i < 8; i++) {
            unsigned short& pixel = sys.gfx[((y + row) % 32) * 64 + (x - i) % 64];
            if (byte & (1 << i)) {
                if (pixel)
                    sys.V[0xF] = true;
                pixel = ~pixel;
            }
        }
    }

    sys.renderer->drawGraphics(sys.gfx);
    sys.pc += 2;
}

void Chip8::CPU::split_E() {
    opcode_table_E[sys.kk()](*this);
}

void Chip8::CPU::SKP_Vx() {
    sys.pc += sys.keypad->keyIsPressed(sys.Vx()) ? 4 : 2;
}

void Chip8::CPU::SKNP_Vx() {
    sys.pc += sys.keypad->keyIsPressed(sys.Vx()) ? 2 : 4;
}

void Chip8::CPU::split_F() {
    opcode_table_F[sys.kk()](*this);
}

void Chip8::CPU::LD_Vx_DT() {
    sys.Vx() = sys.delay_timer;
    sys.pc += 2;
}

void Chip8::CPU::LD_Vx_K() {
    sys.Vx() = sys.keypad->waitForInput();
    sys.pc += 2;
}

void Chip8::CPU::LD_DT_Vx() {
    sys.delay_timer = sys.Vx();
    sys.pc += 2;
}

void Chip8::CPU::LD_ST_Vx() {
    sys.sound_timer = sys.Vx();
    sys.pc += 2;
}

void Chip8::CPU::ADD_I_Vx() {
    sys.I += sys.Vx();
    sys.pc += 2;
}

void Chip8::CPU::LD_F_Vx() {
    sys.I = sys.Vx() * 5;
    sys.pc += 2;
}

void Chip8::CPU::LD_B_Vx() {
    unsigned char num = sys.Vx();
    sys.memory[sys.I] = num / 100;
    num %= 100;
    sys.memory[sys.I + 1] = num / 10;
    num %= 10;
    sys.memory[sys.I + 2] = num;
    sys.pc += 2;
}

void Chip8::CPU::LD_I_Vx() {
    std::copy_n(std::execution::par_unseq, sys.V.begin(), sys.X() + 1, sys.memory.begin() + sys.I);
    sys.pc += 2;
}

void Chip8::CPU::LD_Vx_I() {
    std::copy_n(std::execution::par_unseq, sys.memory.begin() + sys.I, sys.X() + 1, sys.V.begin());
    sys.pc += 2;
}

// clang-format off
const std::array<const unsigned char, 0x50> Chip8::font{
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

const std::array<const std::function<void(Chip8::CPU&)>, 0x10> Chip8::CPU::opcode_table {
    &CPU::split_0,		&CPU::JP_addr,			&CPU::CALL_addr,	&CPU::SE_Vx_byte, 
	&CPU::SNE_Vx_byte,	&CPU::SE_Vx_Vy,			&CPU::LD_Vx_byte,	&CPU::ADD_Vx_byte,
    &CPU::split_8,		&CPU::SNE_Vx_Vy,		&CPU::LD_I_addr,	&CPU::JP_0_addr,
	&CPU::RND_Vx_byte,	&CPU::DRW_Vx_Vy_nibble,	&CPU::split_E,		&CPU::split_F
}; 

const std::array<const std::function<void(Chip8::CPU&)>, 0x100> Chip8::CPU::opcode_table_0 {
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

const std::array<const std::function<void(Chip8::CPU&)>, 0x10> Chip8::CPU::opcode_table_8 {
    &CPU::LD_Vx_Vy,		&CPU::OR_Vx_Vy,		&CPU::AND_Vx_Vy,	&CPU::XOR_Vx_Vy,
	&CPU::ADD_Vx_Vy,	&CPU::SUB_Vx_Vy,	&CPU::SHR_Vx,		&CPU::SUBN_Vx_Vy, 
	nullptr,			nullptr,			nullptr,			nullptr, 
	nullptr,			nullptr,			&CPU::SHL_Vx,		nullptr
};

const std::array<const std::function<void(Chip8::CPU&)>, 0x100> Chip8::CPU::opcode_table_E {
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

const std::array<const std::function<void(Chip8::CPU&)>, 0x100> Chip8::CPU::opcode_table_F {
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
