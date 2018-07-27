#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <memory>
#include <SDL_scancode.h>

class Chip8;

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

    void mainLoop();
    // wait until a key is pressed
    uint8_t waitForInput();
    // check if a key is pressed
    bool keyIsPressed(uint8_t key);
    void setTitleBar(std::string title);

private:
    Chip8& chip8;

    // update renderer scale to match window size
    void changeSize();

    std::array<std::atomic_bool, 0x10> keypad_state;

    // pointer to SDL keyboard state
    const std::unique_ptr<const uint8_t> keyboard_state = nullptr;
    const std::unique_ptr<SDL_Window, SDL_Deleter> window;
    const std::unique_ptr<SDL_Renderer, SDL_Deleter> renderer;
    const std::unique_ptr<SDL_Texture, SDL_Deleter> texture;

    static constexpr std::chrono::steady_clock::duration frame_time =
        std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::duration<double, std::milli>(1000.0 / 60.0));
    std::chrono::time_point<std::chrono::steady_clock> frame_start;
    // keys used for the Chip8 keypad
    static const std::array<SDL_Scancode, 0x10> keys;
};
