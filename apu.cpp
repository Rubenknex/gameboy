#include "apu.h"

#include <iostream>
#include "def.h"

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

    ch1_enabled = false;
    ch1_length_counter = 0;
    ch1_envelope_counter = 0;
    ch1_sweep_counter = 0;
    ch1_timer = 0;
    ch1_sequence_index = 0;

    ch2_enabled = false;
    ch2_length_counter = 0;
    ch2_envelope_counter = 0;
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
        sequencer_clock = 0;
    }

    if (sequencer_clock % 2 == 0) {
        ch1_length_counter--;
        ch2_length_counter--;
    }
    if (sequencer_clock % 8 == 7) {
        ch1_envelope_counter--;
        ch2_envelope_counter--;
    }
    if ((sequencer_clock - 2) % 4 == 0) {
        ch1_sweep_counter--;
    }

    if (ch1_enabled && ch1_length_counter == 0) {
        if (GET_BIT(NR14, 6)) {
            ch1_enabled = false;
            std::cout << "disabled ch1 due to length" << std::endl;
        }
    }

    if (ch2_enabled && ch2_length_counter == 0) {
        if (GET_BIT(NR24, 6)) {
            ch2_enabled = false;
            std::cout << "disabled ch2 due to length" << std::endl;
        }
    }


    sequencer_clock++;

    int length = NR21 & 0b11111;

    // Once every 87 CPU cycles, put an audio sample in the queue
    if (sample_timer == DOWNSAMPLE_RATE) {
        sample_timer = 0;

        // Use maximum volume for now
        int duty = (NR11 & 0b11000000) >> 6;
        int ch1_output = sequences[duty][ch1_sequence_index] * 0xF;

        duty = (NR21 & 0b11000000) >> 6;
        int ch2_output = sequences[duty][ch2_sequence_index] * 0xF;

        float ch1_analog = ((float)ch1_output / 15.0) * 2.0 - 1.0;
        float ch2_analog = ((float)ch2_output / 15.0) * 2.0 - 1.0;

        float mixed = 0.0f;

        if (ch1_enabled)
            mixed += ch1_analog;
        if (ch2_enabled)
            mixed += ch2_analog;

        sample_queue[sample_queue_index] = mixed;
        sample_queue_index++;

        // The queue is full, go to the beginning again
        // At this point the queue can be sent for playback
        if (sample_queue_index == SAMPLE_BUFFER_SIZE)
            sample_queue_index = 0;
    }

    sample_timer++;
}