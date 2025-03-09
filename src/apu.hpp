#pragma once
#include "device.hpp"

enum {
    KEY_CTRL1_SQ1 = 0x4000,
    KEY_PERIOD_SQ1_LOW = 0x4002,
    KEY_PERIOD_SQ1_HIGH = 0x4003,
    KEY_CTRL1_SQ2 = 0x42004,
    KEY_PERIOD_SQ2_LOW = 0x4006,
    KEY_PERIOD_SQ2_HIGH = 0x4007,
    KEY_PERIOD_TRI_LOW = 0x400A,
    KEY_PERIOD_TRI_HIGH = 0x400B,
};

const uint8_t APU_LENGTH_COUNTER_LOAD[32] = {10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14, 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

class ApuDevice : public Device {
 public:
    ApuDevice();
    void tick();
    uint8_t get(uint16_t addr);
    void set(uint16_t addr, uint8_t val);

    void get_period_sq1(uint16_t * period, uint8_t * length);
    void get_period_sq2(uint16_t * period, uint8_t * length);
    uint16_t get_period_tri();
    bool is_sq1_fresh();
    bool is_sq2_fresh();

 private:
    bool m_enable_sq1 = false;
    bool m_enable_sq2 = false;
    uint16_t m_period_sq1 = 0;
    uint16_t m_period_sq2 = 0;
    uint16_t m_period_tri = 0;
    uint8_t m_sq1_length = 0;
    uint8_t m_sq2_length = 0;

    bool m_is_sq1_fresh = false;
    bool m_is_sq2_fresh = false;
};
