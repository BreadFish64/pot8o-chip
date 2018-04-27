#include <algorithm>
#include <iostream>
#include <SDL.h>
#include "chip8.h"
#include "main.h"
#include "render.h"

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    std::queue<std::array<std::array<bool, 64>, 32>> buffer;
    std::timed_mutex buffer_mutex;

    bool start;
    std::thread jim([&] {
        Render render(buffer, buffer_mutex);
        start = true;
        render.run();
    });

    while (!start) {
        ;
    }

    Chip8 chip8(buffer, buffer_mutex);

    return 0;
}
