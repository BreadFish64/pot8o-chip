#pragma once
#include <array>
#include <memory>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class Renderer {
public:
    // gives unique_ptr something to use for a destructor
    struct SDL_Deleter {
        void operator()(SDL_Window* p) const;
        void operator()(SDL_Renderer* p) const;
        void operator()(SDL_Texture* p) const;
    };

    explicit Renderer();
    ~Renderer();

    // blit framebuffer to screen
    void drawGraphics(const std::array<uint16_t, 64 * 32>& frame);
    void setTitleBar(std::string title);
    // update renderer scale to match window size
    void changeSize();

private:
    const std::unique_ptr<SDL_Window, SDL_Deleter> window;
    const std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer;
    const std::unique_ptr<SDL_Texture, SDL_Deleter> texture;
};
