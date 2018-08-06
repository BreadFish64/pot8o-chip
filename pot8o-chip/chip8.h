#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <vector>
#include "frontend.h"

class CPU {
public:
    virtual void execute() = 0;
};

class Chip8 {
public:
    explicit Chip8();
    ~Chip8();

    // load game into memory
    void loadGame(std::string path);
    // main loop
    void emulate();
    // changes internal clock rate
    void changeSpeed(signed int diff);

    uint32_t* getFrameBuffer();
    std::mutex& getFrameBufferLock();

    const std::unique_ptr<Frontend> frontend = nullptr;

    std::atomic_bool limitSpeed = true;
    std::atomic_bool paused = false;

private:
    friend class Interpreter;
    friend class Dynarec;

    // system font to be loaded into memory
    static const std::array<uint8_t, 80> font;

    std::unique_ptr<CPU> cpu;

    unsigned long long cycle_count;
    std::string title;
    std::atomic_int target_clock_speed = 60;
    std::atomic<std::chrono::steady_clock::duration> cycle_length;

    // contains pixel information
    std::array<uint32_t, 64 * 32> frame_buffer{};
    std::mutex frame_buffer_lock;

    std::array<uint8_t, 0x1000> memory{};
};

struct Opcode {
    explicit Opcode(uint16_t opcode);
    explicit Opcode();
    ~Opcode();
    uint16_t opcode;

    inline void operator=(uint16_t rhs) {
        opcode = rhs;
    };
    inline operator uint16_t() {
        return opcode;
    };
    // returns first nibble of opcode
    uint8_t op();
    // returns second nibble of opcode
    uint8_t X();
    // returns third nibble of opcode
    uint8_t Y();
    // returns last nibble of opcode
    uint8_t n();
    // returns last byte of opcode
    uint8_t kk();
    // returns last 3 nibbles of opcode
    uint16_t nnn();
};
