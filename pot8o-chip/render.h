#pragma once
#include <array>
#include <mutex>
#include <queue>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class Render {
public:
    explicit Render();
    void drawGraphics(std::array<unsigned short, 64 * 32>& frame);

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};
