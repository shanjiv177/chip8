#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <SDL2/SDL.h>

// Constants
const int MEMORY_SIZE = 4096;
const int REGISTER_COUNT = 16;
const int STACK_SIZE = 16;
const int KEYPAD_SIZE = 16;
const int DISPLAY_WIDTH = 64;
const int DISPLAY_HEIGHT = 32;
const int PROGRAM_START = 0x200;
const int FONTSET_SIZE = 80;
const int PIXEL_SCALE = 10;
class Chip8
{
public:
    Chip8()
    {
        memory.fill(0);
        V.fill(0);
        stack.fill(0);
        display.fill(0);
        keypad.fill(0);

        // Load fontset
        for (size_t i = 0; i < FONTSET_SIZE; ++i)
        {
            memory[0x50 + i] = fontset[i];
        }

        // Reset
        delayTimer = 0;
        soundTimer = 0;
        pc = PROGRAM_START;
        I = 0;
        sp = 0;
    }

    void loadROM(const std::string &filename);
    void emulateCycle();
    void renderDisplay(SDL_Renderer *renderer);
    void setKeyState(uint8_t key, bool pressed);

    bool shouldDraw() const
    {
        return drawFlag;
    }

private:
    // Components
    std::array<uint8_t, MEMORY_SIZE> memory{};
    std::array<uint8_t, REGISTER_COUNT> V{};
    uint16_t I = 0;
    uint16_t pc = PROGRAM_START;
    std::array<uint16_t, STACK_SIZE> stack{};
    uint16_t sp = 0; // Stack pointer

    std::array<uint8_t, DISPLAY_WIDTH * DISPLAY_HEIGHT> display{};
    std::array<uint8_t, KEYPAD_SIZE> keypad{};

    uint8_t delayTimer = 0;
    uint8_t soundTimer = 0;

    bool drawFlag = false;

    const std::array<uint8_t, FONTSET_SIZE> fontset = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    uint16_t fetchOpcode();
    void executeOpcode(uint16_t opcode);
};

void Chip8::loadROM(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open())
    {
        std::cerr << "Failed to open ROM: " << filename << std::endl;
        return;
    }

    std::streamsize size = file.tellg();

    if (size > MEMORY_SIZE) {
        std::cerr << "ROM FILE size exceeded limits" << std::endl;
        exit(1);
    }

    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size))
    {
        for (size_t i = 0; i < buffer.size(); ++i)
        {
            memory[PROGRAM_START + i] = buffer[i];
        }
    }

    file.close();
}

uint16_t Chip8::fetchOpcode()
{
    return (memory[pc] << 8) | memory[pc + 1];
}

void Chip8::executeOpcode(uint16_t opcode)
{

    const int x = (opcode & 0x0F00) >> 8;
    const int y = (opcode & 0x00F0) >> 4;
    switch (opcode & 0xF000)
    {
    case 0x0000:
        if (opcode == 0x00E0)
        {
            display.fill(0);
            drawFlag = true;
            pc += 2;
        }
        else if (opcode == 0x00EE)
        {
            --sp;
            pc = stack[sp];
            pc += 2;
        }
        break;
    case 0x1000:
        pc = opcode & 0x0FFF;
        break;
    case 0x2000:
        stack[sp] = pc;
        ++sp;
        pc = opcode & 0x0FFF;
        break;
    case 0x3000:
        if (V[(opcode & 0x0F00) >> 8] == (opcode & 0x00FF))
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    case 0x4000:
        if (V[(opcode & 0xF00) >> 8] == (opcode & 0x00FF))
        {
            pc += 2;
        }
        else
        {
            pc += 4;
        }
        break;
    case 0x5000:
        if (V[(opcode & 0x0F00) >> 8] == V[(opcode & 0x00F0) >> 4])
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    case 0x6000:
        V[(opcode & 0x0F00) >> 8] = opcode & 0x00FF;
        pc += 2;
        break;
    case 0x7000:
        V[(opcode & 0x0F00) >> 8] += opcode & 0x00FF;
        pc += 2;
        break;
    case 0x8000:
        switch (opcode & 0x000F)
        {
        case 0x0:
            V[x] = V[y];
            pc += 2;
            break;

        case 0x1:
            V[x] |= V[y];
            pc += 2;
            break;

        case 0x2:
            V[x] &= V[y];
            pc += 2;
            break;

        case 0x3:
            V[x] ^= V[y];
            pc += 2;
            break;

        case 0x4:
        {
            uint16_t sum = V[x] + V[y];
            V[0xF] = (sum > 0xFF) ? 1 : 0;
            V[x] = sum & 0xFF;
            pc += 2;
            break;
        }

        case 0x5: // 8xy5 - SUB Vx, Vy
            V[0xF] = (V[x] > V[y]) ? 1 : 0;
            V[x] -= V[y];
            pc += 2;
            break;

        case 0x6: // 8xy6 - SHR Vx
            V[0xF] = V[x] & 0x1;
            V[x] >>= 1;
            pc += 2;
            break;

        case 0x7: // 8xy7 - SUBN Vx, Vy
            V[0xF] = (V[y] > V[x]) ? 1 : 0;
            V[x] = V[y] - V[x];
            pc += 2;
            break;

        case 0xE: // 8xyE - SHL Vx
            V[0xF] = (V[x] & 0x80) >> 7;
            V[x] <<= 1;
            pc += 2;
            break;

        default:
            std::cerr << "Unknown 0x8 instruction: 0x" << std::hex << opcode << std::endl;
            break;
        }
        break;
    case 0x9000:
        if (V[(opcode & 0x0F00) >> 8] != V[(opcode & 0x00F0) >> 4])
        {
            pc += 4;
        }
        else
        {
            pc += 2;
        }
        break;
    case 0xA000:
        I = opcode & 0x0FFF;
        pc += 2;
        break;
    case 0xB000:
        pc = (opcode & 0x0FFF) + V[0];
        break;
    case 0xC000:
        V[(opcode & 0x0F00) >> 8] = (rand() % 256) & (opcode & 0x00FF);
        pc += 2;
        break;
    case 0xD000:
    {
        uint8_t x = V[(opcode & 0x0F00) >> 8] % DISPLAY_WIDTH;
        uint8_t y = V[(opcode & 0x00F0) >> 4] % DISPLAY_HEIGHT;
        uint8_t height = opcode & 0x000F;
        V[0xF] = 0;

        for (int row = 0; row < height; ++row)
        {
            uint8_t sprite = memory[I + row];
            for (int col = 0; col < 8; ++col)
            {
                if (sprite & (0x80 >> col))
                {
                    size_t index = (y + row) * DISPLAY_WIDTH + (x + col);
                    if (display[index] == 1)
                    {
                        V[0xF] = 1;
                    }
                    display[index] ^= 1;
                }
            }
        }

        drawFlag = true;
        pc += 2;
        break;
    }
    case 0xE000:
        switch (opcode & 0x00FF)
        {
        case 0x009E:
            if (keypad[V[(opcode & 0x0F00) >> 8]])
            {
                pc += 4;
            }
            else
            {
                pc += 2;
            }
            break;
        case 0x00A1:
            if (!keypad[V[(opcode & 0x0F00) >> 8]])
            {
                pc += 4;
            }
            else
            {
                pc += 2;
            }
            break;
        }
        break;
    case 0xF000:
        switch (opcode & 0x00FF)
        {
        case 0x07:
            V[(opcode & 0x0F00) >> 8] = delayTimer;
            pc += 2;
            break;

        case 0x0A:
        {
            bool keyPressed = false;
            for (int i = 0; i < KEYPAD_SIZE; ++i)
            {
                if (keypad[i])
                {
                    V[(opcode & 0x0F00) >> 8] = i;
                    keyPressed = true;
                    break;
                }
            }
            if (!keyPressed)
            {
                return;
            }
            pc += 2;
            break;
        }

        case 0x15:
            delayTimer = V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;

        case 0x18:
            soundTimer = V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;

        case 0x1E:
            I += V[(opcode & 0x0F00) >> 8];
            pc += 2;
            break;

        case 0x29:     
            I = V[(opcode & 0x0F00) >> 8] * 5;
            pc += 2;
            break;

        case 0x33:
        {
            uint8_t value = V[(opcode & 0x0F00) >> 8];
            memory[I] = value / 100;
            memory[I + 1] = (value / 10) % 10;
            memory[I + 2] = value % 10;
            pc += 2;
            break;
        }

        case 0x55:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            for (uint8_t i = 0; i <= x; ++i)
            {
                memory[I + i] = V[i];
            }
            pc += 2;
            break;
        }

        case 0x65:
        {
            uint8_t x = (opcode & 0x0F00) >> 8;
            for (uint8_t i = 0; i <= x; ++i)
            {
                V[i] = memory[I + i];
            }
            pc += 2;
            break;
        }

        default:
            std::cerr << "Unknown 0xF instruction: 0x" << std::hex << opcode << std::endl;
            break;
        }
        break;
    default:
        std::cerr << "Unknown opcode: 0x" << std::hex << opcode << std::endl;
        break;
    }
}

void Chip8::emulateCycle()
{
    uint16_t opcode = fetchOpcode();
    executeOpcode(opcode);

    if (delayTimer > 0)
    {
        --delayTimer;
    }

    if (soundTimer > 0)
    {
        --soundTimer;
    }
}

void Chip8::renderDisplay(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    for (int y = 0; y < DISPLAY_HEIGHT; ++y)
    {
        for (int x = 0; x < DISPLAY_WIDTH; ++x)
        {
            if (display[y * DISPLAY_WIDTH + x])
            {
                SDL_Rect pixel = {x * PIXEL_SCALE, y * PIXEL_SCALE, PIXEL_SCALE, PIXEL_SCALE};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }

    SDL_RenderPresent(renderer);
    drawFlag = false;
}

void Chip8::setKeyState(uint8_t key, bool pressed)
{
    if (key < KEYPAD_SIZE)
    {
        keypad[key] = pressed;
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <ROM file>" << std::endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("CHIP-8 Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DISPLAY_WIDTH * PIXEL_SCALE, DISPLAY_HEIGHT * PIXEL_SCALE, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Chip8 chip8;
    chip8.loadROM(argv[1]);

    bool running = true;
    SDL_Event event;

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
            {
                bool pressed = event.type == SDL_KEYDOWN;
                switch (event.key.keysym.sym)
                {
                case SDLK_1:
                    chip8.setKeyState(0x1, pressed);
                    break;
                case SDLK_2:
                    chip8.setKeyState(0x2, pressed);
                    break;
                case SDLK_3:
                    chip8.setKeyState(0x3, pressed);
                    break;
                case SDLK_4:
                    chip8.setKeyState(0xC, pressed);
                    break;
                case SDLK_q:
                    chip8.setKeyState(0x4, pressed);
                    break;
                case SDLK_w:
                    chip8.setKeyState(0x5, pressed);
                    break;
                case SDLK_e:
                    chip8.setKeyState(0x6, pressed);
                    break;
                case SDLK_r:
                    chip8.setKeyState(0xD, pressed);
                    break;
                case SDLK_a:
                    chip8.setKeyState(0x7, pressed);
                    break;
                case SDLK_s:
                    chip8.setKeyState(0x8, pressed);
                    break;
                case SDLK_d:
                    chip8.setKeyState(0x9, pressed);
                    break;
                case SDLK_f:
                    chip8.setKeyState(0xE, pressed);
                    break;
                case SDLK_z:
                    chip8.setKeyState(0xA, pressed);
                    break;
                case SDLK_x:
                    chip8.setKeyState(0x0, pressed);
                    break;
                case SDLK_c:
                    chip8.setKeyState(0xB, pressed);
                    break;
                case SDLK_v:
                    chip8.setKeyState(0xF, pressed);
                    break;
                }
            }
        }

        chip8.emulateCycle();

        if (chip8.shouldDraw())
        {
            chip8.renderDisplay(renderer);
        }

        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}