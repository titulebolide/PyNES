#include <cstdlib>

#include "ppu.hpp"

PpuDevice::PpuDevice(uint8_t * _chr_rom, Device * cpu_ram) : 
    cpu_ram(cpu_ram), cpu(nullptr), frame(30*8, 32*8, CV_8UC3) {

    for (uint16_t addr = 0; addr < 0x4000; addr ++) {
        chr_rom[addr] = _chr_rom[addr];
    }
}

void PpuDevice::set_cpu(Emu6502 *_cpu) {
    cpu = _cpu;
}

void PpuDevice::set_kb_state(uint8_t kb_state) {
    m_kb_state = kb_state;
}

bool PpuDevice::get_ppuctrl_bit(uint8_t status_bit) {
    return ((ppuctrl & status_bit) != 0);
}

void PpuDevice::inc_ppuaddr() {
    if (get_ppuctrl_bit(PPUCTRL_VRAMINC)) {
        ppuaddr += 32;
    } else {
        ppuaddr += 1;
    }
}

void PpuDevice::set(uint16_t addr, uint8_t value) {

    // if (key < 0x2000) {
    //     key %= 8
    // }
    uint16_t oamdma_source_addr;
    switch (addr)
    {
    case KEY_PPUCTRL:
        ppuctrl = value;
        break;
    
    case KEY_PPUMASK:
        break;
    
    case KEY_PPUADDR:
        // done in two reads : msb, then lsb
        if (ppu_reg_w == 1) {
            //we are reading the lsb
            ppuaddr = (ppuaddr << 8) + static_cast<uint16_t>(value);
            ppu_reg_w = 0;
        } else {
            // msb, null the most signifants two bits (14 bit long addr space)
            ppuaddr = static_cast<uint16_t>(value & 0b00111111);
            ppu_reg_w = 1;
        }
        break;

    case KEY_PPUDATA:
        vram[ppuaddr] = value;
        inc_ppuaddr();
        break;

    case KEY_PPUSCROLL:
        if (value != 0) {
            throw std::runtime_error("scroll not implemented");
        }
        break;

    case KEY_OAMADDR:
        break;

    case KEY_OAMDATA:
        break;

    case KEY_OAMDMA:
        // todo : emulate cpu cycles for DMA ?
        oamdma_source_addr = (static_cast<uint16_t>(value) << 8);
        for (uint16_t i = 0; i < 256; i ++) {
            ppuoam[i] = cpu_ram->get(oamdma_source_addr + i);
        }
        break;

    case KEY_CTRL1:
        controller_strobe = (value & 1); // get lsb
        if (controller_strobe == 1) {
            controller_read_no = 0;
        }
        break;

    default:
        break;
    }
}

uint8_t PpuDevice::get(uint16_t addr) {
    uint8_t retval;
    uint8_t controller_state;
    switch (addr) {
    case KEY_PPUDATA:
        // get buffer value
        retval = ppudata_buffer;
        // update buffer AFTER the read
        ppudata_buffer = vram[ppuaddr];
        // increase ppuaddr after access
        inc_ppuaddr();
        break;
    
    case KEY_PPUSTATUS:
        // TODO : implement PPUSTATUS for real
        retval = 0b10000000;
        break;
    
    case KEY_CTRL1:
        // TODO : In the NES and Famicom, the top three (or five) bits are not driven, and so retain the bits of the previous byte on the bus. Usually this is the most significant byte of the address of the controller port—0x40. Certain games (such as Paperboy) rely on this behavior and require that reads from the controller ports return exactly $40 or $41 as appropriate. See: Controller reading: unconnected data lines.
        controller_state = m_kb_state; // TODO link to a keyboard this

        if (controller_read_no > 7) {
            retval = 1;
        }
        else {
            retval = ((controller_state >> controller_read_no) & 1);
            if (controller_strobe == 0) {
                controller_read_no += 1;
            }
        }
        break;
        
    default:
        break;
    }
    return retval;
}

void PpuDevice::tick() {
    ntick += 1;
    if (ntick % 89342 == 89341){
        ntick = 0;
        if (get_ppuctrl_bit(PPUCTRL_VBLANKNMI)) {
            cpu->interrupt(false);
        }
    }
}


void PpuDevice::render_nametable(cv::Mat * frame) {
    // # x is left to right
    // # y is up to down
    // # but for imshow x is up to down, y is left to right
    for (int8_t sprite_y = 0; sprite_y < 30; sprite_y++) {
        for (int8_t sprite_x = 0; sprite_x < 32; sprite_x++) {
            uint16_t nametable_no = ppuctrl & 0b11;
            uint16_t nametable_base_addr = 0x2000 + 0x400*nametable_no;
            uint8_t sprite_no = vram[nametable_base_addr + sprite_x + sprite_y * 32];
            /*
            7654 3210
            |||| ||++- Color bits 3-2 for top left quadrant of this byte
            |||| ++--- Color bits 3-2 for top right quadrant of this byte
            ||++------ Color bits 3-2 for bottom left quadrant of this byte
            ++-------- Color bits 3-2 for bottom right quadrant of this byte
            */
           uint16_t attribute_table_addr = 0x3c0 +  (static_cast<uint16_t>(sprite_y) / 4)*8 + static_cast<uint16_t>(sprite_x) / 4;
           uint8_t attr_bitshift = 0;
            if (sprite_y % 4 > 1) {
                // bottom
                attr_bitshift += 4;
            }
            if (sprite_x % 4 > 1) {
                // right
                attr_bitshift += 2;
            }
            uint8_t palette_no = ((vram[nametable_base_addr + attribute_table_addr] >> attr_bitshift) & 0b11);
            bool table_no = get_ppuctrl_bit(PPUCTRL_BGPATTTABLE);
            add_sprite(frame, sprite_no, table_no, sprite_x*8, sprite_y*8, palette_no, false, false, false);
        }
    }
}

void PpuDevice::render_oam(cv::Mat * frame) {
    
    bool spritesize = get_ppuctrl_bit(PPUCTRL_SPRITESIZE);

    if (!spritesize) {

        for (int8_t i = 0; i < 64; i++) {
            uint8_t sprite_y = ppuoam[i*4]; // top to bottom
            uint8_t sprite_no = ppuoam[i*4+1];
            uint8_t sprite_attr = ppuoam[i*4+2];
            uint8_t sprite_x = ppuoam[i*4+3]; // left to right
            if (sprite_y == 255) {
                // TODO : I guess this should be handled differently!
                continue;
            }
            bool hflip = ((sprite_attr & PPUOAM_ATT_HFLIP) != 0);
            bool vflip = ((sprite_attr & PPUOAM_ATT_VFLIP) != 0);
            uint8_t palette_no = (sprite_attr & 0b11) + 4; // add 4 to reach OAM palette
            bool table_no = get_ppuctrl_bit(PPUCTRL_OAMPATTTABLE);
            add_sprite(frame, sprite_no, table_no, sprite_x, sprite_y, palette_no, hflip, vflip, true);
        }
    } else {
        throw std::runtime_error("16x8 tiles not supported yet");
    }
}

void PpuDevice::add_sprite(cv::Mat * frame, uint8_t sprite_no, bool table_no, uint8_t sprite_x, uint8_t sprite_y, uint8_t palette_no, bool hflip, bool vflip, bool transparent_bg) { //(self, sprite_no, sprite_table_no, frame, spritex, spritey, palette_no, palettes, hflip, vflip):
    // palette = palettes[palette_no*4:palette_no*4 + 4]
    uint8_t sprite[8][8];
    get_sprite(sprite, sprite_no, table_no, false);

    for (uint8_t x = 0; x < 8; x++) {
        for (uint8_t y = 0; y < 8; y++) {
            uint8_t pix_color = 0;
            if (!hflip && !vflip) {
                pix_color = sprite[y][x];
            } else if (!vflip) { // only hflip
                pix_color = sprite[y][7-x];
            } else if (!hflip) { // only vflip
                pix_color = sprite[7-y][x];
            } else { // vflip and hflip
                pix_color = sprite[7-y][7-x];
            }
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0; // TODO : set here transparent bg
            if (pix_color != 0) {
                // 0x3f00 : palettes location in vram
                // a palette : a set of 4 colors (4 bytes then)
                // palette_no : the index of the palette in the palette list
                // pix_color : the color in the palette
                uint8_t color_no = vram[0x3f00 + static_cast<uint16_t>(palette_no) * 4 + static_cast<uint16_t>(pix_color)];
                r = NES_COLORS[color_no][0]; // TODO : get pointer
                g = NES_COLORS[color_no][1];
                b = NES_COLORS[color_no][2];
            } else if (transparent_bg) {
                continue;
            }
            // TODO there are fatser ways to populate a frame
            frame->at<cv::Vec3b>(sprite_y + y, sprite_x + x) = cv::Vec3b(r, g, b);
        }
    }
}

void PpuDevice::get_sprite(uint8_t sprite[8][8], uint8_t sprite_no, bool table_no, bool doubletile) {
    /*
    sprite is a uint8_t[8][8];
    doubletile : 16x8 tile mode
    */
    if (doubletile) {
        throw std::runtime_error("doubletile not implemented yet");
    }

    uint16_t plane0_addr = (sprite_no + 256*table_no) << 4;
    for (uint8_t j = 0; j < 8; j++) {
        uint8_t plane0 = chr_rom[plane0_addr + j];
        uint8_t plane1 = chr_rom[plane0_addr + j + 8];
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t color0 = (plane0 >> i) & 1;
            uint8_t color1 = (plane1 >> i) & 1;
            uint8_t color = (color1 << 1) + color0;
            sprite[j][7-i] = color;
        }
    }
}

void PpuDevice::render() {
    render_nametable(&frame);
    render_oam(&frame);
    // cv::imshow("prout", frame);
    // cv::waitKey(1);
}

cv::Mat * PpuDevice::getFrame() {
    return &frame;
}
