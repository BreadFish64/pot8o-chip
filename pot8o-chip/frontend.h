#pragma once
#include <array>
#include <memory>
#include <SDL_scancode.h>

struct SDL_Renderer;
struct SDL_Texture;
struct SDL_Window;

class Frontend {
public:
    // gives unique_ptr something to use for a destructor
    struct SDL_Deleter {
        void operator()(SDL_Window* p) const;
        void operator()(SDL_Renderer* p) const;
        void operator()(SDL_Texture* p) const;
    };

    explicit Frontend(Chip8* chip8);
    ~Frontend();

    // wait until a key is pressed
    uint8_t waitForInput();
    // check if a key is pressed
    bool keyIsPressed(uint8_t key);
    // check input between cycles
    void checkInput();

    // blit framebuffer to screen
    void drawGraphics(const std::array<uint16_t, 64 * 32>& frame);
    void setTitleBar(std::string title);
    // update renderer scale to match window size
    void changeSize();

private:
    // keys used for the Chip8 keypad
    static const std::array<const SDL_Scancode, 0x10> keys;

    Chip8& chip8;

    // pointer to SDL keyboard state
    const std::unique_ptr<const uint8_t> keyboard_state = nullptr;
    const std::unique_ptr<SDL_Window, SDL_Deleter> window;
    const std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer;
    const std::unique_ptr<SDL_Texture, SDL_Deleter> texture;
};
