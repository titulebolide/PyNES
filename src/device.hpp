#pragma once

#include <cstdint>
#include <stdexcept>

class Device {
public:
    virtual uint8_t get(uint16_t addr) = 0;
    virtual void set(uint16_t addr, uint8_t val) = 0;
};

class CartridgeRomDevice : public Device {
 private:
    uint8_t mem[0x8000];
    uint16_t m_base_addr;

 public:
    CartridgeRomDevice(uint8_t * prg_rom, uint16_t base_addr) : m_base_addr(base_addr) {
        for (uint16_t addr = 0; addr < 0x8000; addr ++) {
            mem[addr] = prg_rom[addr];
        }
    }

    uint8_t get(uint16_t addr) {
        return mem[addr - m_base_addr];
    }

    void set(uint16_t addr, uint8_t val) {
        throw std::runtime_error("Rom don't support assignment");
    }
};


class RamDevice : public Device {
 private:
    uint8_t mem[0x8000];
    uint16_t m_base_addr;

 public:
    RamDevice(uint16_t base_addr) : m_base_addr(base_addr) {
    }

    uint8_t get(uint16_t addr) {
        return mem[addr - m_base_addr];
    }

    void set(uint16_t addr, uint8_t val) {
        mem[addr - m_base_addr] = val;
    }
};
