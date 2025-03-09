#include <iostream>
#include <chrono>
#include "audio.hpp"


#include "lstdebugger.hpp"
#include "cpu.hpp"
#include "utils.hpp"
#include "device.hpp"
#include "ppu.hpp"

#include <opencv2/opencv.hpp>
#include <SDL.h>

#include <iostream>
#include <thread>

#include <signal.h>
#include <map>

std::map<char,uint8_t> CONTROLLER_MAPPING = {{'p', 0}, {'o', 1}, {'b', 2}, {'n', 3}, {'z', 4}, {'s', 5}, {'q', 6}, {'d', 7}}; // A, B, Select, Start, Up, Down, Left, Right

void turn_bit_off(uint8_t * value, uint8_t bit) {
    *value &= ~(1 << bit);
}

void turn_bit_on(uint8_t * value, uint8_t bit) {
    *value |= (1 << bit);
}


void ui(PpuDevice * ppu) {
    struct sigaction action;
    sigaction(SIGINT, NULL, &action);
    SDL_Init(SDL_INIT_EVERYTHING);
    if (SDL_Init(SDL_INIT_NOPARACHUTE) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return;
    }
    sigaction(SIGINT, &action, NULL);

    Beeper b;

    if(!SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0"))
    {
        std::cout << "SDL can not disable compositor bypass!" << std::endl;
        return;
    }

    SDL_Window* window = SDL_CreateWindow("Display Image", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 32*8*2, 30*8*2, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_INPUT_FOCUS);
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, 32*8, 30*8);
    if (texture == nullptr) {
        std::cerr << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return;
    }

    cv::Mat * frame = ppu->getFrame();
    
    bool quit = false;

    uint8_t kb_state = 0;

    int n = 0;

    while(!quit) {
        SDL_Event e;
        uint8_t kb_status = 0;
        // Wait indefinitely for the next available event
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
            if (e.type == SDL_KEYDOWN | e.type == SDL_KEYUP) {
                uint8_t keycode = 0;
                try {
                    keycode = CONTROLLER_MAPPING.at(e.key.keysym.sym);
                } catch(const std::out_of_range& ex) {
                    continue;
                }
                if (e.type == SDL_KEYDOWN) {
                    turn_bit_on(&kb_state, keycode);
                } else {
                    turn_bit_off(&kb_state, keycode);
                }
            }
        }
        // std::cout << ppu->get_period_sq1() << std::endl;
        if (ppu->is_sq1_fresh()) {
            uint16_t period;
            uint8_t length;
            ppu->get_period_sq1(&period, &length);
            std::cout << "sq1: " << period << " " << static_cast<int>(length) << std::endl;
            b.setFrequency1(1789773 /  (16.0f*( static_cast<float>(period) + 1)), static_cast<float>(length)/240.0f);
        }
        if (ppu->is_sq2_fresh()) {
            uint16_t period;
            uint8_t length;
            ppu->get_period_sq2(&period, &length);
            std::cout << "sq2: " << period << " " << static_cast<int>(length) << std::endl;
            b.setFrequency2(1789773 /  (16.0f*( static_cast<float>(period) + 1)), static_cast<float>(length)/240.0f);
        }
        
        n++;
        if (n % 4 == 0) {
            n = 0;

            ppu->set_kb_state(kb_state);
            ppu->render();

            SDL_UpdateTexture(texture, nullptr, frame->data, frame->step1());
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr);
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(10);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void run(Emu6502 * cpu, PpuDevice * ppu, bool * kill) {
    unsigned long long loopCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();
    while (!(*kill)) {
        cpu->tick();
        ppu->tick();
        ppu->tick();
        ppu->tick();
        loopCount++;

        if (loopCount % 100000 == 0) {
            // slow down !
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }

        // if (loopCount % 200000 == 0) {
        //     auto currentTime = std::chrono::high_resolution_clock::now();
        //     std::chrono::duration<double> elapsed = currentTime - startTime;
        //     double loopsPerSecond = loopCount / elapsed.count();
        //     std::cout << "Loops per second: " << loopsPerSecond << std::endl;
        //     loopCount = 0;
        //     startTime = currentTime;
        // }
    }
}

int main() {
    uint8_t prg[0x8000] = {0};
    uint8_t chr[0x4000] = {0}; // TODO : check sizes

    parseInes("../rom/Donkey-Kong-NES-Disassembly/dk.nes", prg, chr);
    LstDebuggerAsm6 lst("../rom/Donkey-Kong-NES-Disassembly/dk.lst", true);

    CartridgeRomDevice rom(prg);
    RamDevice ram;
    PpuDevice ppu(chr, &ram);

    Memory mem({
        {0x0000, &ram},
        {0x2000, &ppu},
        {0xc000, &rom},
    });


    Emu6502 cpu(&mem, false, &lst);
    ppu.set_cpu(&cpu); // urgh

    unsigned long long loopCount = 0;
    auto startTime = std::chrono::high_resolution_clock::now();


    bool kill = false;
    std::thread t1(run, &cpu, &ppu, &kill); 

    ui(&ppu);

    kill = true;

    t1.join();
    
    return 0;
}
