#pragma once
#include <array>
#include <memory>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class Renderer {
    friend class Keypad;

public:
    struct SDL_Deleter {
        void operator()(SDL_Window* p) const;
        void operator()(SDL_Renderer* p) const;
        void operator()(SDL_Texture* p) const;
    };

    explicit Renderer();
    ~Renderer();

    void drawGraphics(std::array<unsigned short, 64 * 32>& frame);
    void setTitleBar(std::string title);
    void changeSize(signed int diff);

private:
    int scale = 8;

    std::unique_ptr<SDL_Window, SDL_Deleter> window;
    std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer;
    std::unique_ptr<SDL_Texture, SDL_Deleter> texture;
};
