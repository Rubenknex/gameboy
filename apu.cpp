#include "apu.h"

#include <iostream>
#include "def.h"
#include "fmt/format.h"

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

    sequencer_clock = 0;
    sequencer_step = 0;

    ch1_enabled = false;
    ch1_length_counter = 0;
    ch1_envelope_counter = 0;
    ch1_volume = 0;
    ch1_sweep_counter = 0;
    ch1_timer = 0;
    ch1_sequence_index = 0;

    ch2_enabled = false;
    ch2_length_counter = 0;
    ch2_envelope_counter = 0;
    ch2_volume = 0;
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

    // Update the sequencer clock, every 8192 cycles -> 512 Hz
    sequencer_clock++;
    if (sequencer_clock == 8192) {
        sequencer_clock = 0;

        // Update the current sequencer step 
        sequencer_step++;
        if (sequencer_step == 8)
            sequencer_step = 0;
    }

    // Decrease length counters every other sequencer step -> 256 Hz
    if (sequencer_step % 2 == 0) {
        ch1_length_counter--;
        ch2_length_counter--;
    }

    // Update envelope every 7th sequencer step -> 64 Hz
    if (sequencer_step == 7) {
        ch1_envelope_counter--;
        if (ch1_envelope_counter == 0) {
            if (GET_BIT(NR12, 3) && ch1_volume < 15) {
                ch1_volume++;
            }
            else if (GET_BIT(NR12, 3)==0 && ch1_volume > 0) {
                ch1_volume--;
            }
        }

        ch2_envelope_counter--;
        if (ch2_envelope_counter == 0) {
            if (GET_BIT(NR22, 3) && ch2_volume < 15) {
                ch2_volume++;
            }
            else if (GET_BIT(NR22, 3)==0 && ch2_volume > 0) {
                ch2_volume--;
            }
        }
    }
    // Update sweep every 4th sequencer step -> 128 Hz
    if ((sequencer_step - 2) % 4 == 0) {
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



    // Once every 87 CPU cycles, put an audio sample in the queue
    if (sample_timer == DOWNSAMPLE_RATE) {
        sample_timer = 0;

        // Get the current square wave duty cycle pattern
        int pattern = (NR11 & 0b11000000) >> 6;
        // Go from 0 to 1 (pattern) to (-1 to 1) * (volume, 0-7) * enabled
        int ch1_output = ((sequences[pattern][ch1_sequence_index] * 2) - 1) * ch1_volume * ch1_enabled;

        pattern = (NR21 & 0b11000000) >> 6;
        int ch2_output = ((sequences[pattern][ch2_sequence_index] * 2) - 1) * ch2_volume * ch2_enabled;

        // Convert channel outputs to (-1.0, 1.0) range and sum
        float mixed = (float)ch1_output / 7.0f + (float)ch2_output / 7.0f

        sample_queue[sample_queue_index] = mixed;
        sample_queue_index++;

        // The queue is full, go to the beginning again
        // At this point the queue can be sent for playback
        if (sample_queue_index == SAMPLE_BUFFER_SIZE)
            sample_queue_index = 0;
    }

    sample_timer++;
}