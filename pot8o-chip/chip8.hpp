#pragma once
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <thread>
#include <vector>

class Chip8 {
public:
    using Frame = std::array<std::uint64_t, 32>;
    struct Interface;

    class CPU {
        friend Chip8;
        virtual void Run(Chip8::Interface& interface, std::vector<std::uint8_t> game) = 0;
    };

    struct Interface {
        Frame frame_buffer{};
        std::array<std::atomic_bool, 16> keypad_state;
        std::atomic_uint64_t cycle_count;
        std::atomic_uint8_t delay_timer;
        std::atomic_uint8_t sound_timer;
        std::atomic_bool send_frame = true;
        std::atomic_bool stop_flag = false;

        void PushFrameBuffer(Frame& frame) {
            std::copy(frame.begin(), frame.end(), frame_buffer.begin());
            send_frame = false;
        }
    };

private:
    std::unique_ptr<Interface> interface;
    std::unique_ptr<CPU> cpu;
    std::optional<std::thread> cpu_thread, timer_thread;

    // returns true if sound_timer hits 0
    bool DecrementTimers() {
        if (interface->delay_timer)
            interface->delay_timer--;
        std::uint8_t st = interface->sound_timer;
        if (st != 0)
            interface->sound_timer--;
        return st == 1;
    }

public:
    Chip8(std::unique_ptr<CPU> cpu) : cpu{std::move(cpu)} {}

    void Run(std::vector<std::uint8_t> game) {
        Stop();
        interface = std::make_unique<Interface>();
        assert(interface);
        timer_thread = std::thread([this] {
            for (;;) {
                DecrementTimers();
                if (interface->stop_flag)
                    return;
                std::this_thread::sleep_for(std::chrono::duration<double>(1. / 60.));
            }
        });
        cpu_thread =
            std::thread([this, game = std::move(game)] { cpu->Run(*interface, std::move(game)); });
    }

    void Stop() {
        if (interface)
            interface->stop_flag = true;
        if (cpu_thread)
            cpu_thread->join();
        if (timer_thread)
            timer_thread->join();
        cpu_thread.reset();
        timer_thread.reset();
        interface.reset();
    }

    std::uint64_t GetCycles() {
        return interface->cycle_count.exchange(0) / 2;
    }

    void SetKey(std::size_t key, bool val) {
        interface->keypad_state[key] = val;
    }

    void ConsumeFrameBuffer(std::function<void(const Frame&)> callback) {
        if (interface->send_frame)
            return;
        callback(interface->frame_buffer);
        interface->send_frame = true;
    }
};
