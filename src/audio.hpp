#pragma once
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <cmath>
#include <iostream>

const int AMPLITUDE = 28000;
const int SAMPLE_RATE = 44100;
const float SAMPLE_RATE_PEROOD = 1.0f / SAMPLE_RATE;
const int BASE_FREQUENCY = 440;  // Base frequency in Hz (A4 note)
const float MODULATION_FREQUENCY = 10;  // Frequency of the frequency modulation in Hz

class Beeper
{
private:
    double phase1;  // Tracks the phase of the sine wave
    double phase2;  // Tracks the phase of the sine wave
    Uint32 start_time;  // Tracks the start time for modulation
    float m_frequency_1;
    float m_frequency_2;
    double m_sq1_cur_dur;
    double m_sq2_cur_dur;

public:
    Beeper();
    ~Beeper();
    void generateSamples(Sint16 *stream, int length);
    double getInstantaneousFrequency();
    void setFrequency1(float frequency, float duration);  
    void setFrequency2(float frequency, float duration);
};
