#include "apu.hpp"

ApuDevice::ApuDevice() {
    return;
}

uint8_t ApuDevice::get(uint16_t addr) {
    return 0;
}

void ApuDevice::set(uint16_t addr , uint8_t value) {
    uint8_t retval;

    switch (addr)
    {
    case KEY_CTRL1_SQ1:
        m_enable_sq1 = ((value & 0b00100000) == 0);
        break;
    case KEY_PERIOD_SQ1_LOW:
        m_period_sq1 = (m_period_sq1 & 0xff00) | static_cast<uint16_t>(value);
        break;
    
    case KEY_PERIOD_SQ1_HIGH:
        m_period_sq1 = (static_cast<uint16_t>(value & 0b111) << 8) | (m_period_sq1 & 0x00ff);
        m_sq1_length = APU_LENGTH_COUNTER_LOAD[(value & 0b11111000) >> 3];
        m_is_sq1_fresh = true;
        break;

    case KEY_CTRL1_SQ2:
        m_enable_sq1 = ((value & 0b00100000) == 0);
        break;
    
    case KEY_PERIOD_SQ2_LOW:
        m_period_sq2 = (m_period_sq2 & 0xff00) | static_cast<uint16_t>(value);
        break;
    
    case KEY_PERIOD_SQ2_HIGH:
        m_period_sq2 = (static_cast<uint16_t>(value & 0b111) << 8) | (m_period_sq2 & 0x00ff);
        m_sq2_length = APU_LENGTH_COUNTER_LOAD[(value & 0b11111000) >> 3];
        m_is_sq2_fresh = true;
        break;

    case KEY_PERIOD_TRI_LOW:
        m_period_tri = (m_period_tri & 0xff00) | static_cast<uint16_t>(value);
        break;
    
    case KEY_PERIOD_TRI_HIGH:
        m_period_tri = (static_cast<uint16_t>(value & 0b111) << 8) | (m_period_tri & 0x00ff);
        break;

    default:
        break;
    }

    return ;
}

void ApuDevice::tick() {
    return;
}

void ApuDevice::get_period_sq1(uint16_t * period, uint8_t * length) {
    m_is_sq1_fresh = false;
    *period = m_period_sq1;
    *length = m_sq1_length;
}

void ApuDevice::get_period_sq2(uint16_t * period, uint8_t * length) {
    m_is_sq2_fresh = false;
    *period = m_period_sq2;
    *length = m_sq2_length;
}

uint16_t ApuDevice::get_period_tri() {
    return m_period_tri;
}

bool ApuDevice::is_sq1_fresh() {
    return m_is_sq1_fresh;
}

bool ApuDevice::is_sq2_fresh() {
    return m_is_sq2_fresh;
}
