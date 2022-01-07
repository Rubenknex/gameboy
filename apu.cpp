#include "apu.h"

#include <iostream>

#include "gameboy.h"

const int sequences[4][8] = {
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
};

APU::APU(GameBoy* gb) : gb(gb) {
    wave_pattern.resize(16);

    sample_timer = 0;
    sample_queue_index = 0;
    sample_queue.resize(SAMPLE_BUFFER_SIZE);

    ch1_timer = 0;
    ch1_sequence_index = 0;

    ch2_timer = 0;
    ch2_sequence_index = 0;
}

APU::~APU() {

}

void APU::cycle() {
    int ch1_freq = ((NR14 & 0b111) << 8) | NR13;
    int ch2_freq = ((NR24 & 0b111) << 8) | NR23;

    // Happens every 1/8th of the square wave period
    if (ch1_timer == (2048 - ch1_freq) * 4) {
        ch1_sequence_index++;
        if (ch1_sequence_index == 8)
            ch1_sequence_index = 0;

        ch1_timer = 0;
    }
    ch1_timer++;

    // Happens every 1/8th of the square wave period
    if (ch2_timer == (2048 - ch2_freq) * 4) {
        ch2_sequence_index++;
        if (ch2_sequence_index == 8)
            ch2_sequence_index = 0;

        ch2_timer = 0;
    }
    ch2_timer++;

    if (sequencer_clock == 8192) {
        // do stuff

        sequencer_clock = 0;
    }

    sequencer_clock++;

    if (sample_timer == DOWNSAMPLE_RATE) {
        sample_timer = 0;

        int duty = (NR11 & 0b11000000) >> 6;
        int ch1_output = sequences[duty][ch1_sequence_index];

        duty = (NR21 & 0b11000000) >> 6;
        int ch2_output = sequences[duty][ch2_sequence_index];

        sample_queue[sample_queue_index] = ch1_output * 3 + ch2_output * 3;
        sample_queue_index++;


        if (sample_queue_index == SAMPLE_BUFFER_SIZE)
            sample_queue_index = 0;
    }

    sample_timer++;
}