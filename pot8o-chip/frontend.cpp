#include <chrono>
#include <fstream>
#include <thread>
#include <type_traits>
#include <fmt/format.h>

#include <SDL.h>

#include "chip8.hpp"
#include "frontend.hpp"
#include "interpreter.hpp"
#include "llvm_aot.hpp"
#include "open_gl.hpp"

constexpr auto WIDTH = 64, HEIGHT = 32;

SDLFrontend::SDLFrontend() : chip8(std::make_unique<LLVMAOT>()) {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = decltype(window)(
        SDL_CreateWindow("pot8o chip", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH * 8,
                         HEIGHT * 8,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI),
        SDL_Deleter());
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
#if _DEBUG
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
    context = std::make_unique<GLContext>(window.get());
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress))
        fmt::print("OpenGL failed to load");
    SDL_GL_SetSwapInterval(1);

#if _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGL::DebugHandler, nullptr);
#endif

    // setup drawing
    frame_buffer.Create();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, frame_buffer);
    present_program.Create({OpenGL::quad_source}, {OpenGL::bpp_frag});
    glUseProgram(present_program);
    vao.Create();
    glBindVertexArray(vao);
    vertex_buffer.Create();
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    static constexpr GLfloat v_data[12]{0., 0., 1., 0., 0., 1., 0., 1., 1., 0., 1., 1.};
    glBufferData(GL_ARRAY_BUFFER, sizeof(v_data), v_data, GL_STATIC_DRAW);
    GLuint a_pos = glGetAttribLocation(present_program, "a_pos");
    glEnableVertexAttribArray(a_pos);
    glVertexAttribPointer(a_pos, 2, GL_FLOAT, false, 0, 0);

    /*
    renderer = std::unique_ptr<SDL_Renderer, SDL_Deleter>(
        SDL_CreateRenderer(window.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC),
    SDL_Deleter()); texture = std::unique_ptr<SDL_Texture, SDL_Deleter>(
        SDL_CreateTexture(renderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
                          WIDTH, HEIGHT),
        SDL_Deleter());*/
}

SDLFrontend::~SDLFrontend() {
    SDL_Quit();
};

void SDLFrontend::LoadGame(std::string& path) {
    {
        std::ifstream game(path, std::ios::binary);
        if (!game) {
            fmt::print("bad game path: {}", path);
            return;
        }
        chip8.Run(std::vector<std::uint8_t>(std::istreambuf_iterator<char>(game),
                                            std::istreambuf_iterator<char>()));
    }

    SDL_DisplayMode display_mode;
    SDL_GetWindowDisplayMode(window.get(), &display_mode);
    SDL_Event event;
    std::string title;
    for (;;) {
        chip8.ConsumeFrameBuffer([this](auto frame) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8UI, sizeof(frame[0]) / sizeof(std::uint8_t),
                         frame.size(), 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, frame.data());

            glDrawArrays(GL_TRIANGLES, 0, 6);
            SDL_GL_SwapWindow(window.get());
        });
        // SDL_UpdateTexture(texture.get(), nullptr, pixel_data.data(), 256);
        // SDL_RenderCopy(renderer.get(), texture.get(), nullptr, nullptr);
        // SDL_RenderPresent(renderer.get());

        title = fmt::format("pot8o chip - {:0=.2} GHz",
                            chip8.GetCycles() * display_mode.refresh_rate / 1'000'000'000.);
        SDL_SetWindowTitle(window.get(), title.data());

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                auto key = key_map.find(event.key.keysym.sym);
                if (key != key_map.end())
                    chip8.SetKey(key->second, event.key.state);
            } break;
            case SDL_WINDOWEVENT: {
                int w, h;
                SDL_GetWindowSize(window.get(), &w, &h);
                glViewport(0, 0, w, h);
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

void SDLFrontend::SDL_Deleter::operator()(SDL_GLContext* p) const {
    SDL_GL_DeleteContext(p);
}
