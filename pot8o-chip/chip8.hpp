#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

class Chip8 {
public:
    using Frame = std::array<std::uint64_t, 32>;
    class Interface;

    class CPU {
        friend Chip8;
        virtual void Run(Chip8::Interface& interface, std::vector<std::uint8_t> game) = 0;
    };

    class Interface {
    public:
        /// returns true if sound_timer hits 0
        bool DecrementTimers() {
            if (delay_timer)
                delay_timer--;
            auto st = sound_timer.load();
            if (st != 0)
                sound_timer--;
            return st == 1;
        }

        void ConsumeFrameBuffer(std::function<void(const Frame&)> callback) {
            if (send_frame)
                return;
            callback(frame_buffer);
            send_frame = true;
        }

        void PushFrameBuffer(Frame& frame) {
            std::copy(frame.begin(), frame.end(), frame_buffer.begin());
            send_frame = false;
        }

        inline bool GetSendFrame() {
            return send_frame;
        }

    private:
        Frame frame_buffer{};

    public:
        std::array<std::atomic_bool, 16> keypad_state;

    private:
        std::atomic_bool send_frame = true;

    public:
        std::atomic_uint8_t delay_timer;
        std::atomic_uint8_t sound_timer;
        std::atomic_uint64_t cycle_count;
    } interface;

private:
    std::unique_ptr<CPU> cpu;
    bool running = false;

public:
    Chip8(std::unique_ptr<CPU> cpu) : cpu{std::move(cpu)} {}

    void Run(std::vector<std::uint8_t> game) {
        if (!running) {
            running = true;
            cpu->Run(interface, game);
        }
    }
};
