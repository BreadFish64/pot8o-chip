#include <iostream>
#include <string>
#include <thread>
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include "chip8.h"
#include "main.h"

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    // get path from CLI otherwise wait for input
    std::string path;
    if (argc <= 1) {
        std::cin >> path;
    } else {
        path = argv[1];
    }

    Chip8 chip8;
    chip8.loadGame(path);
    std::thread emu_thread([&chip8] { chip8.emulate(); });
    chip8.frontend->mainLoop();

    return 0;
}
