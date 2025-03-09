#pragma once

#include <opencv2/opencv.hpp> 

#include "device.hpp"
#include "cpu.hpp"


enum {
	KEY_PPUCTRL = 0x2000,
	KEY_PPUMASK = 0x2001,
	KEY_PPUSTATUS = 0x2002,
	KEY_OAMADDR = 0x2003,
	KEY_OAMDATA = 0x2004,
	KEY_PPUSCROLL = 0x2005,
	KEY_PPUADDR = 0x2006,
	KEY_PPUDATA = 0x2007,
	KEY_OAMDMA = 0x4014,
	KEY_CTRL1 = 0x4016,
    KEY_CTRL2 = 0x4017,
};


static const uint8_t PPUCTRL_VBLANKNMI     = 0b10000000; // 0 : disable NMI on vblank
static const uint8_t PPUCTRL_SPRITESIZE    = 0b00100000; // 0 : 8x8, 1: 8x16
static const uint8_t PPUCTRL_BGPATTTABLE   = 0b00010000; // Pattern table no select in 8x8 mode
static const uint8_t PPUCTRL_OAMPATTTABLE  = 0b00001000; // Pattern table no select in 8x8 mode
static const uint8_t PPUCTRL_VRAMINC       = 0b00000100;

static const uint8_t PPUOAM_ATT_HFLIP = 0b01000000;
static const uint8_t PPUOAM_ATT_VFLIP = 0b10000000;

const uint8_t NES_COLORS[64][3] = {{124, 124, 124}, {0, 0, 252}, {0, 0, 188}, {68, 40, 188}, {148, 0, 132}, {168, 0, 32}, {168, 16, 0}, {136, 20, 0}, {80, 48, 0}, {0, 120, 0}, {0, 104, 0}, {0, 88, 0}, {0, 64, 88}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {188, 188, 188}, {0, 120, 248}, {0, 88, 248}, {104, 68, 252}, {216, 0, 204}, {228, 0, 88}, {248, 56, 0}, {228, 92, 16}, {172, 124, 0}, {0, 184, 0}, {0, 168, 0}, {0, 168, 68}, {0, 136, 136}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {248, 248, 248}, {60, 188, 252}, {104, 136, 252}, {152, 120, 248}, {248, 120, 248}, {248, 88, 152}, {248, 120, 88}, {252, 160, 68}, {248, 184, 0}, {184, 248, 24}, {88, 216, 84}, {88, 248, 152}, {0, 232, 216}, {120, 120, 120}, {0, 0, 0}, {0, 0, 0}, {252, 252, 252}, {164, 228, 252}, {184, 184, 248}, {216, 184, 248}, {248, 184, 248}, {248, 164, 192}, {240, 208, 176}, {252, 224, 168}, {248, 216, 120}, {216, 248, 120}, {184, 248, 184}, {184, 248, 216}, {0, 252, 252}, {248, 216, 248}, {0, 0, 0}, {0, 0, 0}};


class PpuDevice : public Device {
private:
    uint8_t chr_rom[0x4000];

    // TODO : this is quite bad, we share here cpuram for OAMDMA
    Device * cpu_ram;
    // this is used to call the interrupt, same, could do better (interface ?)
    Emu6502 * cpu;

    // cpu_interrupt = None
    // cpu_ram = None

    uint8_t vram[0x4000] = {0}; // 14 bit addr space
    long ntick = 0;
    uint8_t ppu_reg_w = 0;
    uint16_t ppuaddr = 0;
    uint8_t ppuctrl = 0;
    uint8_t ppustatus = 0;
    uint8_t ppuoam[256] = {0};
    uint8_t ppudata_buffer = 0; // ppudata does not read directly ram but a buffer that is updated after each read

    uint8_t controller_strobe = 0;
    uint8_t controller_read_no = 0;

    uint8_t m_kb_state = 0;

    cv::Mat frame;
    
    bool get_ppuctrl_bit(uint8_t status_bit);

    void inc_ppuaddr();
    void render_oam(cv::Mat * frame);
    void render_nametable(cv::Mat * frame);
    void add_sprite(cv::Mat * frame, uint8_t sprite_no, bool table_no, uint8_t sprite_x, uint8_t sprite_y, uint8_t palette_no, bool hflip, bool vflip, bool transparent_bg);
    void get_sprite(uint8_t sprite[8][8], uint8_t sprite_no, bool table_no, bool doubletile);

 public:
    PpuDevice(uint8_t * chr_rom, Device * cpu_ram);
    uint8_t get(uint16_t addr);
    void set(uint16_t addr, uint8_t val);
    void tick();
    void set_cpu(Emu6502 * cpu);
    void set_kb_state(uint8_t kb_state);
    void render();
    cv::Mat *getFrame();
};
