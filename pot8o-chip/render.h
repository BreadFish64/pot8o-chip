#pragma once
#include <array>
#include <mutex>
#include <queue>

class SDLRenderer;
class SDLTexture;
class SDLWindow;

class Render {
public:
    explicit Render(std::queue<std::array<std::array<bool, 64>, 32>>& buffer,
                    std::timed_mutex& buffer_mutex);
    void run();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Texture* texture = nullptr;

    std::queue<std::array<std::array<bool, 64>, 32>>& buffer;
    std::timed_mutex& buffer_mutex;

    void drawGraphics(std::array<std::array<bool, 64>, 32> frame);
};
