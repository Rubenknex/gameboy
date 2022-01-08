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
    sample_queue.resize(APU_BUFFER_SIZE);

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

u8 APU::read_byte(u16 address) {
    u8 result = 0;

    switch (address & 0xFF) {
        case 0x10: // Channel 1 sweep
            result = NR10;
            break;
        case 0x11: // Channel 1 length/wave pattern duty
            result = NR11;
            break;
        case 0x12: // Channel 1 envelope
            result = NR12;
            break;
        case 0x13: // Channel 1 frequency low
            result = 0;
            break;
        case 0x14: // Channel 1 frequency high
            result = NR14;
            break;
        case 0x16: // Channel 2 length/wave pattern duty
            result = NR21;
            break;
        case 0x17: // Channel 2 envelope
            result = NR22;
            break;
        case 0x18: // Channel 2 frequency low
            result = 0;
            break;
        case 0x19: // Channel 2 frequency high
            result = NR24;
            break;
        case 0x1A: // Channel 3 on/off
            result = NR30;
            break;
        case 0x1B: // Channel 3 length
            result = 0;
            break;
        case 0x1C: // Channel 3 output level
            result = NR32;
            break;
        case 0x1D: // Channel 3 frequency low
            result = 0;
            break;
        case 0x1E: // Channel 3 frequency high
            result = NR34;
            break;
        case 0x20: // Channel 4 length
            result = 0;
            break;
        case 0x21: // Channel 4 envelope
            result = NR42;
            break;
        case 0x22: // Channel 4 polynomial counter
            result = NR43;
            break;
        case 0x23: // Channel 4 counter/consecutive
            result = NR44;
            break;
        case 0x24: // Channel control
            result = NR50;
            break;
        case 0x25: // Sound output terminal
            result = NR51;
            break;
        case 0x26: // Sound on/off
            result = NR52;
            break;
        case 0x30: // Waveform storage5

            break;
    }
    
    return result;
}

void APU::write_byte(u16 address, u8 value) {
    switch (address & 0xFF) {
        case 0x10: // Channel 1 sweep
            NR10 = value;
            break;
        case 0x11: {// Channel 1 length/wave pattern duty
            NR11 = value;
            int length_cycles = (64 - (value & 0b11111)) * (16376);
            ch1_length_counter = length_cycles;
            } break;
        case 0x12: // Channel 1 envelope
            NR12 = value;
            ch1_volume = (value & 0b11110000) >> 4;
            ch1_envelope_counter = value & 0b111;
            break;
        case 0x13: // Channel 1 frequency low
            NR13 = value;
            // Reset the timer and position in waveform, because this
            // is probably a new sound being played
            ch1_timer = 0;
            ch1_sequence_index = 0;
            break;
        case 0x14: // Channel 1 frequency high
            NR14 = value;
            ch1_timer = 0;
            ch1_sequence_index = 0;
            if (GET_BIT(value, 7)) {
                // Restart sound
                ch1_length_counter = (64 - (NR11 & 0b11111)) * (16376);
                ch1_enabled = true;
            }
            break;
        case 0x16: { // Channel 2 length/wave pattern duty
            NR21 = value;
            int length_cycles = (64 - (value & 0b11111)) * (16376);
            ch2_length_counter = length_cycles;
            } break;
        case 0x17: // Channel 2 envelope
            NR22 = value;
            ch2_volume = (value & 0b11110000) >> 4;
            ch2_envelope_counter = value & 0b111;
            break;
        case 0x18: // Channel 2 frequency low
            NR23 = value;
            ch2_timer = 0;
            ch2_sequence_index = 0;
            break;
        case 0x19: // Channel 2 frequency high
            NR24 = value;
            ch2_timer = 0;
            ch2_sequence_index = 0;
            
            if (GET_BIT(value, 7)) {
                // Restart sound
                ch2_length_counter = (64 - (NR21 & 0b11111)) * (16376);
                ch2_enabled = true;
            }
            break;
        case 0x1A: // Channel 3 on/off

            break;
        case 0x1B: // Channel 3 length

            break;
        case 0x1C: // Channel 3 output level

            break;
        case 0x1D: // Channel 3 frequency low

            break;
        case 0x1E: // Channel 3 frequency high

            break;
        case 0x20: // Channel 4 length

            break;
        case 0x21: // Channel 4 envelope

            break;
        case 0x22: // Channel 4 polynomial counter

            break;
        case 0x23: // Channel 4 counter/consecutive

            break;
        case 0x24: // Channel control

            break;
        case 0x25: // Sound output terminal

            break;
        case 0x26: // Sound on/off

            break;
        case 0x30: // Waveform storage

            break;
    }
}

void APU::cycle() {
    // The frequency value from the registers, not the actual frequency
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
            // If volume should be increased and we are not at max yet
            if (GET_BIT(NR12, 3) && ch1_volume < 15) {
                ch1_volume++;
            }
            // If volume should be decreased and we are not at zero yet
            else if (GET_BIT(NR12, 3)==0 && ch1_volume > 0) {
                ch1_volume--;
            }

            // Reset the envelope counter
            ch1_envelope_counter = NR11 & 0b111;
        }

        ch2_envelope_counter--;
        if (ch2_envelope_counter == 0) {
            if (GET_BIT(NR22, 3) && ch2_volume < 15) {
                ch2_volume++;
            }
            else if (GET_BIT(NR22, 3)==0 && ch2_volume > 0) {
                ch2_volume--;
            }

            ch2_envelope_counter = NR21 & 0b111;
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
        float mixed = (float)ch1_output / 7.0f + (float)ch2_output / 7.0f;

        sample_queue[sample_queue_index] = mixed;
        sample_queue_index++;

        // The queue is full, go to the beginning again
        // At this point the queue can be sent for playback
        if (sample_queue_index == APU_BUFFER_SIZE)
            sample_queue_index = 0;
    }

    sample_timer++;
}