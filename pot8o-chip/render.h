#pragma once
#include <array>
#include <mutex>
#include <queue>

class SDL_Renderer;
class SDL_Texture;
class SDL_Window;

class Render {
public:
    explicit Render();
    void drawGraphics(std::array<bool, 64 * 32> frame);

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;
};
