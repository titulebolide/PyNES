#include "audio.hpp"

void audio_callback(void*, Uint8*, int);

Beeper::Beeper()
{
    SDL_AudioSpec desiredSpec;

    desiredSpec.freq = SAMPLE_RATE;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 256;
    desiredSpec.callback = audio_callback;
    desiredSpec.userdata = this;

    SDL_AudioSpec obtainedSpec;

    if (SDL_OpenAudio(&desiredSpec, &obtainedSpec) < 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        exit(1);
    }

    // Initialize phase and start time
    phase1 = 0;
    phase2 = 0;
    start_time = SDL_GetTicks();

    m_frequency_1 = 400;
    m_frequency_2 = 400;

    m_sq1_cur_dur = 0;
    m_sq2_cur_dur = 0;

    // Start playing audio
    SDL_PauseAudio(0);
}

Beeper::~Beeper()
{
    SDL_CloseAudio();
}

void Beeper::setFrequency1(float frequency, float duration)
{
    m_frequency_1 = frequency;
    m_sq1_cur_dur = duration;
}

void Beeper::setFrequency2(float frequency, float duration)
{
    m_frequency_2 = frequency;
    m_sq2_cur_dur = duration;
}


void Beeper::generateSamples(Sint16 *stream, int length)
{
    for (int i = 0; i < length; i++) {
        
        stream[i] = 0;
        if (m_sq1_cur_dur > 0) {
            stream[i] += (AMPLITUDE/3) * (phase1 < M_PI ? 1:-1);
            m_sq1_cur_dur -= 1/SAMPLE_RATE;
        }
        if (m_sq2_cur_dur > 0) {
            stream[i] += (AMPLITUDE/3) * (phase2 < M_PI ? 1:-1);
            m_sq2_cur_dur -= SAMPLE_RATE_PEROOD;
        }
        // if (phase_tri < M_PI) {
        //     stream[i] += (AMPLITUDE/6) * (2 * phase_tri / M_PI - 1) ;
        // } else {
        //     stream[i] += (AMPLITUDE/6) * (3 - 2 * phase_tri / M_PI);
        // }
        phase1 += 2 * M_PI * m_frequency_1 / SAMPLE_RATE;
        phase2 += 2 * M_PI * m_frequency_2 / SAMPLE_RATE;

        // Wrap phase to avoid overflow
        if (phase1 > 2 * M_PI) {
            phase1 -= 2 * M_PI;
        }
        if (phase2 > 2 * M_PI) {
            phase2 -= 2 * M_PI;
        }
    }
}

void audio_callback(void *_beeper, Uint8 *_stream, int _length)
{
    Sint16 *stream = (Sint16*) _stream;
    int length = _length / 2;
    Beeper* beeper = (Beeper*) _beeper;

    beeper->generateSamples(stream, length);
}
