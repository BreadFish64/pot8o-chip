#pragma once
#include <array>
#include <memory>

#include "chip8.hpp"

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class SDLFrontend {
public:
    /// gives unique_ptr something to use for a destructor
    struct SDL_Deleter {
        void operator()(SDL_Window* p) const;
        void operator()(SDL_Renderer* p) const;
        void operator()(SDL_Texture* p) const;
    };

    explicit SDLFrontend();
    ~SDLFrontend();

    void LoadGame(std::string& path);

private:
	void ExplodeFrame(const Chip8::Frame& frame);
    std::array<std::uint32_t, 64 * 32> pixel_data{};

    const std::unique_ptr<SDL_Window, SDL_Deleter> window;
    const std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer;
    const std::unique_ptr<SDL_Texture, SDL_Deleter> texture;
    Chip8 chip8;
};
