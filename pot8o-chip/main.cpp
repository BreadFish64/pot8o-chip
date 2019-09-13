#include <iostream>
#include <string>
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "frontend.hpp"

int main(int argc, char* argv[]) {
    // get path from CLI otherwise wait for input
    std::string path;
    if (argc <= 1) {
        std::cin >> path;
    } else {
        path = argv[1];
    }

    SDL_Init(SDL_INIT_EVERYTHING);
    SDLFrontend frontend;
    while (true) {
        frontend.LoadGame(path);
        std::cin >> path;
    }
    return 0;
}
