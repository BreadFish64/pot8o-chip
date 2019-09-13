#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <type_traits>

#include <glad/glad.h>
#include <SDL.h>

#include "chip8.hpp"
#include "frontend.hpp"
#include "interpreter.hpp"
#include "llvm_aot.hpp"

constexpr auto WIDTH = 64, HEIGHT = 32;

SDLFrontend::SDLFrontend()
    : window{std::unique_ptr<SDL_Window, SDL_Deleter>(
          SDL_CreateWindow("pot8o chip", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE),
          SDL_Deleter())},
      chip8(std::make_unique<LLVMAOT>()) {}

SDLFrontend::~SDLFrontend() = default;

void SDLFrontend::LoadGame(std::string& path) {
    using namespace std::literals::chrono_literals;

    std::ifstream game(path, std::ios::binary);
    if (!game) {
        std::cout << "Bad game file: " << path;
        return;
    }
    std::thread emu_thread([bin = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(game),
                                                            std::istreambuf_iterator<char>()),
                            this] { chip8.Run(bin); });

    std::chrono::high_resolution_clock::time_point frame_start;
    SDL_Event event;
    std::string title;
    for (;;) {
        frame_start = std::chrono::high_resolution_clock::now();

        chip8.interface.DecrementTimers();
        chip8.interface.ConsumeFrameBuffer([this](auto frame) { ExplodeFrame(frame); });
        title = "pot8o chip " + std::to_string(chip8.interface.cycle_count / 2 * 60) + " Hz";
        chip8.interface.cycle_count = 0;
        SDL_SetWindowTitle(window.get(), title.data());



        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key = key_map.find(event.key.keysym.sym);
                if (key != key_map.end())
                    chip8.interface.keypad_state[key->second] = event.key.state;
            } break;
            case SDL_QUIT:
                return;
            }
        }

        std::this_thread::sleep_until(frame_start + std::chrono::duration<double>(1. / 60.));
    }
}

void SDLFrontend::ExplodeFrame(const Chip8::Frame& frame) {
    using PixelType = std::remove_reference_t<decltype(pixel_data[0])>;
    constexpr auto stride = sizeof(frame[0]) * CHAR_BIT;
    static_assert(sizeof(frame) * CHAR_BIT * sizeof(PixelType) == sizeof(pixel_data),
                  "frame sizes do not match");

    for (auto i = 0; i < frame.size(); i++) {
        const auto row = frame[i];
        for (auto j = 0; j < stride; j++)
            pixel_data[i * stride + j] =
                (row & (decltype(row)(1) << (64 - j))) ? ~PixelType(0) : PixelType(0);
    }
}

void SDLFrontend::SDL_Deleter::operator()(SDL_Window* p) const {
    SDL_DestroyWindow(p);
}

void SDLFrontend::SDL_Deleter::operator()(SDL_Renderer* p) const {
    SDL_DestroyRenderer(p);
}

void SDLFrontend::SDL_Deleter::operator()(SDL_Texture* p) const {
    SDL_DestroyTexture(p);
}
