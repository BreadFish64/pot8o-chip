#include <chrono>
#include <fstream>
#include <thread>
#include <type_traits>
#include <fmt/format.h>
#include <fmt/printf.h>

#include <SDL.h>
#include <glad/glad.h>

#include "chip8.hpp"
#include "frontend.hpp"
#include "interpreter.hpp"
#include "llvm_aot.hpp"

constexpr auto WIDTH = 64, HEIGHT = 32;

SDLFrontend::SDLFrontend() : chip8(std::make_unique<LLVMAOT>()) {
    window = std::unique_ptr<SDL_Window, SDL_Deleter>(
        SDL_CreateWindow("pot8o chip", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH * 8,
                         HEIGHT * 8, SDL_WINDOW_RESIZABLE),
        SDL_Deleter());
    renderer = std::unique_ptr<SDL_Renderer, SDL_Deleter>(
        SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC), SDL_Deleter());
    texture = std::unique_ptr<SDL_Texture, SDL_Deleter>(
        SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                          WIDTH, HEIGHT),
        SDL_Deleter());
}

SDLFrontend::~SDLFrontend() = default;

void SDLFrontend::LoadGame(std::string& path) {
    using namespace std::literals::chrono_literals;
    {
        std::ifstream game(path, std::ios::binary);
        if (!game) {
            fmt::printf("bad game path: {}", path);
            return;
        }
        chip8.Run(std::vector<std::uint8_t>(std::istreambuf_iterator<char>(game),
                                            std::istreambuf_iterator<char>()));
    }

    SDL_GL_SetSwapInterval(1);

    SDL_DisplayMode display_mode;
    SDL_GetWindowDisplayMode(window.get(), &display_mode);
    SDL_Event event;
    std::string title;
    std::uint64_t frame_count = 0;
    for (;;) {
        chip8.ConsumeFrameBuffer([this](auto frame) { ExplodeFrame(frame); });
        SDL_UpdateTexture(texture.get(), nullptr, pixel_data.data(), 256);
        SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
        SDL_RenderPresent(renderer.get());

        title = fmt::format("pot8o chip - {:0=.2} GHz", chip8.GetCycles() * display_mode.refresh_rate / 1'000'000'000.);
        SDL_SetWindowTitle(window.get(), title.data());

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key = key_map.find(event.key.keysym.sym);
                if (key != key_map.end())
                    chip8.SetKey(key->second, event.key.state);
            } break;
            case SDL_QUIT:
                chip8.Stop();
                return;
            }
        }
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
                (row & (decltype(row)(1) << (63 - j))) ? ~PixelType(0) : PixelType(0);
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
