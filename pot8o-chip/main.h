#pragma once
class SDL_Renderer;
class SDL_Texture;
class Chip8;

void drawGraphics(SDL_Renderer* renderer, SDL_Texture* texture, Chip8 chip8);
