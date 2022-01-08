#ifndef APU_H
#define APU_H

#include <vector>

#include "def.h"

/*
Two outputs SO1 (right side) and SO2 (left side)
Input Vin, routed to one or both outputs

Four channels:
Ch1: square wave with sweep and envelope
Ch2: square wave with envelope
Ch3: Wave patterns from wave RAM
Ch4: White noise with envelope

Simplest channel: 2 (square wave)
4 different duty cycles (12.5%, 25%, 50%, 75%)
8-step sequence, like ____---- (50%)

f = 131072 / (2048-x) Hz with x an 11-bit number (up to 2047)
So the frequency can be 64 Hz to 131072 Hz

Now how many CPU cycles is a period of the square wave?
#cycles = f_cpu / f_square = 4192304 / (131072/(2048-x))
                           = (2048-x) * 32

And so a single step of the duty cycle is (2048-x)*4 CPU cycles

So cycle the APU for every CPU cycle

Use a 48kHz output sample rate
4192304/48000 = ~87
So only use 1 out of 87 audio samples

Gather about 1000 samples before outputting using SDL_QueueAudio

x=1800 -> 500 Hz

On the screen: 256 samples at 48 kHz

1s = 48000 samples
500 Hz -> 0.002s -> 9.6 samples, should be visible
*/

#define DOWNSAMPLE_RATE 87
#define SAMPLE_BUFFER_SIZE 1024

class GameBoy;

class APU {
public:
    APU(GameBoy* gb);
    ~APU();

    void cycle();

public:
    GameBoy* gb;

    int sample_timer;
    int sample_queue_index;
    std::vector<float> sample_queue;

    int sequencer_clock;
    int sequencer_step;

    // Channel 1: Tone & sweep
    bool ch1_enabled;
    int ch1_length_counter;
    int ch1_envelope_counter;
    int ch1_volume;
    int ch1_sweep_counter;
    int ch1_timer;
    int ch1_sequence_index;
    u8 NR10; // FF10, Ch1 sweep register
    u8 NR11; // FF11, Ch1 sound length / wave pattern duty
    u8 NR12; // FF12, Ch1 volume envelope
    u8 NR13; // FF13, Ch1 frequency lo
    u8 NR14; // FF14, Ch1 frequency hi

    // Channel 2: Tone
    bool ch2_enabled;
    int ch2_length_counter;
    int ch2_envelope_counter;
    int ch2_volume;
    int ch2_timer;
    int ch2_sequence_index;
    u8 NR21; // FF11, Ch1 sound length / wave pattern duty
    u8 NR22; // FF12, Ch1 volume envelope
    u8 NR23; // FF13, Ch1 frequency lo
    u8 NR24; // FF14, Ch1 frequency hi

    // Channel 3: Wave output
    u8 NR30; // FF1A, sound on/off
    u8 NR31; // FF1B, sound length
    u8 NR32; // FF1C, select output level
    u8 NR33; // FF1D, frequency lo
    u8 NR34; // FF1E, frequency hi

    // Channel 4: Noise
    u8 NR41; // FF20, sound length
    u8 NR42; // FF21, volume envelope
    u8 NR43; // FF22, polynomial counter
    u8 NR44; // FF23, counter/consecutive;initial

    // Control registers
    u8 NR50; // FF24, channel control / ON-OFF / volume
    u8 NR51; // FF25, select sound output terminal
    u8 NR52; // FF26. sound on/off

    // FF30-FF3F, wave pattern RAM, 32 4-bit samples
    std::vector<u8> wave_pattern;
};

#endif