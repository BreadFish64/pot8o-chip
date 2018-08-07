#include <iostream>
#include <string>
#include <thread>
#include <windows.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "chip8.h"
#include "dynarec.h"
#include "main.h"

#define XBYAK_NO_OP_NAMES
#include <xbyak_util.h>

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_EVERYTHING);

    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    ChunkOfCode::page_size = system_info.dwPageSize;

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
