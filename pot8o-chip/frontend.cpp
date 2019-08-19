#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>
#include <type_traits>
#include <SDL.h>

#include "chip8.hpp"
#include "frontend.hpp"
#include "interpreter.hpp"
#include "llvm_aot.hpp"

constexpr auto WIDTH = 64, HEIGHT = 32;

SDLFrontend::SDLFrontend()
    : window{std::unique_ptr<SDL_Window, SDL_Deleter>(
          SDL_CreateWindow("pot8o chip", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                           WIDTH * 8, HEIGHT * 8, SDL_WINDOW_RESIZABLE),
          SDL_Deleter())},
      renderer{std::unique_ptr<SDL_Renderer, SDL_Deleter>(
          SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED), SDL_Deleter())},
      texture{std::unique_ptr<SDL_Texture, SDL_Deleter>(
          SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                            WIDTH, HEIGHT),
          SDL_Deleter())},
      chip8{Chip8(std::make_unique<LLVMAOT>())} {}

SDLFrontend::~SDLFrontend() = default;

void SDLFrontend::LoadGame(std::string& path) {
    using namespace std::literals::chrono_literals;

    std::ifstream game(path, std::ios::binary);
    if (!game) {
        std::cout << "Bad game file: " << path;
        return;
    }
    std::thread emu_thread(
        [bin = std::vector<std::uint8_t>(std::istreambuf_iterator<char>(game),
                                         std::istreambuf_iterator<char>()),
         this] { chip8.Run(bin); });

    std::chrono::high_resolution_clock::time_point frame_start;
    SDL_Event event;

    for (;;) {
        frame_start = std::chrono::high_resolution_clock::now();

        chip8.interface.DecrementTimers();
        chip8.interface.ConsumeFrameBuffer([this](auto frame) { ExplodeFrame(frame); });

        SDL_UpdateTexture(texture.get(), nullptr, pixel_data.data(), 256);
        SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
        SDL_RenderPresent(renderer.get());

        while (SDL_PollEvent(&event)) {
        // handle events
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

/*const std::array<SDL_Scancode, 0x10> SDLFrontend::keys{
    SDL_SCANCODE_KP_0,     SDL_SCANCODE_KP_1,    SDL_SCANCODE_KP_2,      SDL_SCANCODE_KP_3,
    SDL_SCANCODE_KP_4,     SDL_SCANCODE_KP_5,    SDL_SCANCODE_KP_6,      SDL_SCANCODE_KP_7,
    SDL_SCANCODE_KP_8,     SDL_SCANCODE_KP_9,    SDL_SCANCODE_KP_DIVIDE, SDL_SCANCODE_KP_MULTIPLY,
    SDL_SCANCODE_KP_MINUS, SDL_SCANCODE_KP_PLUS, SDL_SCANCODE_KP_ENTER,  SDL_SCANCODE_KP_PERIOD};*/
