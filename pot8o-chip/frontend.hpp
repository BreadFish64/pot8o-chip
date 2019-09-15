#pragma once
#include <array>
#include <map>
#include <memory>

#include "chip8.hpp"
#include "open_gl.hpp"

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
        void operator()(SDL_GLContext* p) const;
    };

    explicit SDLFrontend();
    ~SDLFrontend();

    void LoadGame(std::string& path);

private:
	struct GLContext {
        SDL_GLContext ctx;
		explicit GLContext(SDL_Window* window) {
            ctx = SDL_GL_CreateContext(window);
		}
		~GLContext() {
            SDL_GL_DeleteContext(ctx);
		}
	};
    void ExplodeFrame(const Chip8::Frame& frame);

    // todo: add configuration somehow
    std::map<SDL_Keycode, std::size_t> key_map{
        {SDLK_KP_0, 0x0},      {SDLK_KP_7, 0x1},        {SDLK_KP_8, 0x2},     {SDLK_KP_9, 0x3},
        {SDLK_KP_4, 0x4},      {SDLK_KP_5, 0x5},        {SDLK_KP_6, 0x6},     {SDLK_KP_1, 0x7},
        {SDLK_KP_2, 0x8},      {SDLK_KP_3, 0x9},        {SDLK_RIGHT, 0xA},    {SDLK_KP_PERIOD, 0xB},
        {SDLK_KP_DIVIDE, 0xC}, {SDLK_KP_MULTIPLY, 0xD}, {SDLK_KP_MINUS, 0xE}, {SDLK_KP_PLUS, 0xF},
    };

    std::unique_ptr<SDL_Window, SDL_Deleter> window;
    std::unique_ptr<GLContext> context;
    OpenGL::Texture frame_buffer;
    OpenGL::Program present_program;
    OpenGL::Buffer vertex_buffer;
    OpenGL::VAO vao;
    //std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer;
    //std::unique_ptr<SDL_Texture, SDL_Deleter> texture;

    std::array<std::uint32_t, 64 * 32> pixel_data{};

    Chip8 chip8;
};
