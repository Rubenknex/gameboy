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
    volume = 0.05;
    
    wave_pattern.resize(16);

    sample_timer = 0;
    sample_queue_index = 0;
    sample_queue.resize(APU_BUFFER_SIZE);

    global_timer = 0;
    sequencer_clock = 0;
    sequencer_updated = false;
    sequencer_step = 0;

    ch1_enabled = false;
    ch1_frequency = 1;
    ch1_length_counter = 0;
    ch1_envelope_counter = 0;
    ch1_volume = 0;
    ch1_sweep_counter = 0;
    ch1_timer = 0;
    ch1_sequence_index = 0;

    ch2_enabled = false;
    ch2_frequency = 1;
    ch2_length_counter = 0;
    ch2_envelope_counter = 0;
    ch2_volume = 0;
    ch2_timer = 0;
    ch2_sequence_index = 0;

    ch3_enabled = false;
    ch3_frequency = 1;
    ch3_length_counter = 0;
    ch3_volume = 0;
    ch3_timer = 0;
    ch3_sequence_index = 0;

    ch4_enabled = false;
    ch4_length_counter = 0;
    ch4_envelope_counter = 0;
    ch4_volume = 0;
    ch4_timer = 0;
    ch4_period = 0;
    ch4_lfsr = 0b111111111111111; // 15 bits, initially all are 1
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
        case 0x10: {// Channel 1 sweep
            NR10 = value;

            int time = (value & 0b1110000) >> 4;
            bool decrease = value & 0b1000;
            int sweep_shift = value & 0b111;
            //std::cout << fmt::format("Ch1 sweep time={} decrease={} shift={}", time, decrease, sweep_shift) << std::endl;
            } break;
        case 0x11: // Channel 1 length/wave pattern duty
            NR11 = value;
            ch1_length_counter = (64 - (value & 0b11111));
            //std::cout << "Ch1 length: " << ch1_length_counter << std::endl;
            break;
        case 0x12: // Channel 1 envelope
            NR12 = value;
            ch1_volume = (value & 0b11110000) >> 4;
            ch1_envelope_counter = value & 0b111;
            //std::cout << "Ch1 envelope counter = " << ch1_envelope_counter << std::endl;
            break;
        case 0x13: { // Channel 1 frequency low
            NR13 = value;
            update_frequencies();
            // Reset the timer and position in waveform, because this
            // is probably a new sound being played
            //ch1_timer = 0;
            //ch1_sequence_index = 0;
            //std::cout << "Ch1 set freq low" << std::endl;
            } break;
        case 0x14: // Channel 1 frequency high
            NR14 = value;
            update_frequencies();
            //ch1_timer = 0;
            //ch1_sequence_index = 0;
            if (GET_BIT(value, 7)) {
                // Restart sound
                //ch1_length_counter = (64 - (NR11 & 0b11111)) * (16376);
                ch1_length_counter = (64 - (NR11 & 0b11111));
                ch1_envelope_counter = NR12 & 0b111;
                ch1_sweep_counter = (NR10 & 0b1110000) >> 4;
                ch1_volume = (NR12 & 0b11110000) >> 4;
                ch1_enabled = true;
                ch1_timer = 0;
                //std::cout << fmt::format("Play Ch1: freq={} length={} envelope={} volume={} length_stop={} sweep={}", ch1_frequency, ch1_length_counter, ch1_envelope_counter, ch1_volume, value & 0b1000000, ch1_sweep_counter) << std::endl;
            } else {
                //std::cout << "Ch1 set freq hi" << std::endl;
            }
            break;
        case 0x16: // Channel 2 length/wave pattern duty
            NR21 = value;
            ch2_length_counter = 64 - (value & 0b11111);
            break;
        case 0x17: // Channel 2 envelope
            NR22 = value;
            ch2_volume = (value & 0b11110000) >> 4;
            ch2_envelope_counter = value & 0b111;
            break;
        case 0x18: // Channel 2 frequency low
            NR23 = value;
            update_frequencies();
            break;
        case 0x19: // Channel 2 frequency high
            NR24 = value;
            update_frequencies();

            if (GET_BIT(value, 7)) {
                // Restart sound
                ch2_timer = 0;
                ch2_length_counter = 64 - (NR21 & 0b11111);
                ch2_envelope_counter = NR22 & 0b111;
                ch2_volume = (NR22 & 0b11110000) >> 4;
                ch2_enabled = true;
                //std::cout << fmt::format("Play Ch1: length={} envelope={} volume={}", ch1_length_counter, ch1_envelope_counter, ch1_volume) << std::endl;
            }
            break;
        case 0x1A: // Channel 3 on/off
            NR30 = value;
            ch3_enabled = value & 0b10000000;
            //std::cout << "Wave output: " << (value & 0b10000000) << std::endl;
            break;
        case 0x1B: // Channel 3 length
            NR31 = value;
            ch3_length_counter = 256 - (value & 0b11111);
            break;
        case 0x1C: // Channel 3 output level
            NR32 = value;
            break;
        case 0x1D: // Channel 3 frequency low
            NR33 = value;
            update_frequencies();
            break;
        case 0x1E: // Channel 3 frequency high
            NR34 = value;
            update_frequencies();

            if (GET_BIT(value, 7)) {
                ch3_enabled = true;
                ch3_length_counter = 256 - (value & 0b11111);
                ch3_timer = 0;
                ch3_sequence_index = 0;
            }
            break;
        case 0x20: // Channel 4 length
            NR41 = value;
            ch4_length_counter = 64 - (value & 0b11111);
            break;
        case 0x21: // Channel 4 envelope
            NR42 = value;
            ch4_volume = (value & 0b11110000) >> 4;
            ch4_envelope_counter = value & 0b111;
            break;
        case 0x22: { // Channel 4 polynomial counter
            int shift = value & 0b11110000;
            bool width_mode = value & 0b1000;
            int divisor = value & 0b111;

            float freq = 0.0f;
            if (divisor == 0)
                freq = 524288.0f / 0.5f / (float)(2 << shift);
            else
                freq = 524288.0f / (float)divisor / (float)(2 << shift);

            //freq = freq / 32;

            ch4_period = (int)(4192304.0f / freq);

            /*
            if (divisor == 0)
                ch4_period = 4 * (2 << shift);
            else
                ch4_period = (8 * divisor) * (2 << shift);
            */

            //std::cout << fmt::format("shift={} divisor={} freq={} period={}", shift, divisor, freq, ch4_period) << std::endl;
            } break;
        case 0x23: // Channel 4 counter/consecutive
            NR44 = value;
            ch4_timer = 0;
            //ch4_sequence_index = 0;
            if (GET_BIT(value, 7)) {
                ch4_enabled = true;
                ch4_length_counter = 64 - (value & 0b11111);
                ch4_length_counter = 16;
                ch4_timer = 0;
                ch4_envelope_counter = NR42 & 0b111;
                ch4_volume = (NR42 & 0b11110000) >> 4;
                ch4_lfsr = 0b111111111111111;

                //std::cout << "Playing noise sound"<< std::endl;
                //ch4_length_counter = (64 - (NR41 & 0b11111)) * (16376);

                //std::cout << fmt::format("Play Ch4: length={} envelope={} volume={} length_stop={}", ch4_length_counter, ch4_envelope_counter, ch4_volume, value & 0b1000000) << std::endl;
            }
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

void APU::update_frequencies() {
    int x = ((NR14 & 0b111) << 8) | NR13;
    ch1_frequency = 131072 / (2048 - x);
    //std::cout << "Ch1 frequency: " << ch1_frequency << std::endl;

    x = ((NR24 & 0b111) << 8) | NR23;
    ch2_frequency = 131072 / (2048 - x);

    x = ((NR34 & 0b111) << 8) | NR33;
    ch3_frequency = 65536 / (2048 - x);
    //std::cout << "Ch3 frequency: " << ch3_frequency << std::endl;
}

void APU::cycle() {
    // Happens every 1/8th of the square wave period
    if (ch1_timer == (CLOCK_FREQ / (8 * ch1_frequency))) {
        ch1_sequence_index++;
        if (ch1_sequence_index == 8)
            ch1_sequence_index = 0;

        ch1_timer = 0;
    }
    ch1_timer++;

    // Happens every 1/8th of the square wave period
    if (ch2_timer == (CLOCK_FREQ / (8 * ch2_frequency))) {
        ch2_sequence_index++;
        if (ch2_sequence_index == 8)
            ch2_sequence_index = 0;

        ch2_timer = 0;
    }
    ch2_timer++;

    if (ch3_timer == (CLOCK_FREQ / (8 * ch3_frequency))) {
        ch3_sequence_index++;
        //std::cout << "Ch3 index: " << ch3_sequence_index << std::endl;
        if (ch3_sequence_index > 31)
            ch3_sequence_index = 0;

        ch3_timer = 0;
    }
    ch3_timer++;

    if (ch4_timer == ch4_period) {
        // XOR two lowest bits of the LFSR
        bool result = (ch4_lfsr & 0b01) ^ ((ch4_lfsr & 0b10) >> 1);

        ch4_lfsr = ch4_lfsr >> 1;
        SET_BIT(ch4_lfsr, 14, result);

        // Also set bit 6 of the LFSR
        if (GET_BIT(NR43, 3)) {
            SET_BIT(ch4_lfsr, 6, result);
            std::cout << "7-bit mode" << std::endl;
        }
            

        //std::cout << fmt::format("Updated the LFSR: {:015b}, xor={}", ch4_lfsr, result) << std::endl;

        ch4_timer = 0;
    }
    ch4_timer++;

    // Update the sequencer clock, every 8192 cycles -> 512 Hz
    sequencer_clock++;
    if (sequencer_clock == 8192) {
        sequencer_clock = 0;

        // Update the current sequencer step 
        sequencer_step++;
        sequencer_updated = true;
        
        if (sequencer_step == 8)
            sequencer_step = 0;
    } else{
        sequencer_updated = false;
    }

    // Decrease length counters every other sequencer step -> 256 Hz
    if (sequencer_updated && sequencer_step % 2 == 0) {
        ch1_length_counter--;
        ch2_length_counter--;
        ch3_length_counter--;
        ch4_length_counter--;
        //std::cout << sequencer_step << std::endl;
        //std::cout << "Decreased ch4 length counter: " << ch4_length_counter << " cycle=" << global_timer << std::endl;
    }

    // Update envelope every 7th sequencer step -> 64 Hz
    if (sequencer_updated && sequencer_step == 7) {
        ch1_envelope_counter--;
        //std::cout << "ch1_envelope_counter=" << ch1_envelope_counter << std::endl;
        if (ch1_envelope_counter == 0) {
            // If volume should be increased and we are not at max yet
            if (GET_BIT(NR12, 3) && ch1_volume < 15) {
                ch1_volume++;
            }
            // If volume should be decreased and we are not at zero yet
            else if (GET_BIT(NR12, 3)==0 && ch1_volume > 0) {
                ch1_volume--;
                //std::cout << "Decreased ch1 volume to " << ch1_volume << std::endl;
            }

            // Reset the envelope counter
            ch1_envelope_counter = NR12 & 0b111;
            //std::cout << "Reset ch1_envelope_counter to " << (NR12 & 0b111) << std::endl;
        }

        ch2_envelope_counter--;
        if (ch2_envelope_counter == 0) {
            if (GET_BIT(NR22, 3) && ch2_volume < 15) {
                ch2_volume++;
            }
            else if (GET_BIT(NR22, 3)==0 && ch2_volume > 0) {
                ch2_volume--;
            }

            ch2_envelope_counter = NR22 & 0b111;
        }

        ch4_envelope_counter--;
        if (ch4_envelope_counter == 0) {
            if (GET_BIT(NR42, 3) && ch4_volume < 15) {
                ch4_volume++;
            }
            else if (GET_BIT(NR42, 3)==0 && ch4_volume > 0) {
                ch4_volume--;
            }

            ch4_envelope_counter = NR42 & 0b111;
        }
    }
    // Update sweep every 4th sequencer step -> 128 Hz
    if (sequencer_updated && (sequencer_step - 2) % 4 == 0) {
        ch1_sweep_counter--;
        //std::cout << "Sweep counter: " << ch1_sweep_counter << std::endl;

        if (ch1_sweep_counter == 0) {
            //std::cout << "doign the sweep"<< std::endl;
            int shift = NR10 & 0b111;
            if (GET_BIT(NR10, 3))
                ch1_frequency = ch1_frequency - ch1_frequency / (1 << shift);
            else
                ch1_frequency = ch1_frequency + ch1_frequency / (1 << shift);

            ch1_sweep_counter = (NR10 & 0b1110000) >> 4;
        }
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

    if (ch3_enabled && ch3_length_counter == 0) {
        if (GET_BIT(NR34, 6)) {
            ch3_enabled = false;
            std::cout << "disabled ch3 due to length" << std::endl;
        }
    }

    if (ch4_enabled && ch4_length_counter == 0) {
        if (GET_BIT(NR44, 6)) {
            ch4_enabled = false;
            //std::cout << "disabled ch4 due to length" << std::endl;
        }
    }

    // Once every 87 CPU cycles, put an audio sample in the queue
    if (sample_timer == DOWNSAMPLE_RATE) {
        sample_timer = 0;

        // Get the current square wave duty cycle pattern
        int pattern = (NR11 & 0b11000000) >> 6;
        // Go from (0, 1) pattern to (-15, 15) output value
        int ch1_output = ((sequences[pattern][ch1_sequence_index] * 2) - 1) * ch1_volume * ch1_enabled;

        pattern = (NR21 & 0b11000000) >> 6;
        int ch2_output = ((sequences[pattern][ch2_sequence_index] * 2) - 1) * ch2_volume * ch2_enabled;

        u8 current_byte = gb->mmu.read_byte(0xFF30 + (ch3_sequence_index >> 1));
        //if (ch3_enabled)
        //    std::cout << fmt::format("mem={:04X}", ch3_sequence_index >> 1) << std::endl;
        u8 sample = 0;
        if (ch3_sequence_index & 0b1)
            sample = current_byte & 0x0F;
        else
            sample = current_byte >> 8;

        u8 output_level = NR32 & 0b1100000;

        int ch3_output = (sample) * ch3_enabled;

        if (output_level == 0)
            ch3_output = 0;

        int ch4_output = (GET_BIT(ch4_lfsr, 0) * 2 - 1) * ch4_volume * ch4_enabled;

        // Convert channel outputs to (-1.0, 1.0) range and sum
        float mixed = (float)ch1_output / 15.0f + (float)ch2_output / 15.0f + (float)ch3_output / 15.0F + (float)ch4_output / 15.0f;
        //float mixed = (float)ch4_output / 15.0f;

        // Clip because summing can go above 1.0
        if (mixed < -1.0f) {
            //std::cout << "clipped - " << ch4_output << " " << mixed << " " << GET_BIT(ch4_lfsr, 0) << std::endl;
            mixed = -1.0f;
        }
            
        if (mixed > 1.0f) {
            //std::cout << "clipped + " << ch4_output << " " << mixed << std::endl;
            mixed = 1.0f;
        }
            

        // Global volume control
        mixed *= volume;

        sample_queue[sample_queue_index] = mixed;
        sample_queue_index++;

        // The queue is full, go to the beginning again
        // At this point the queue can be sent for playback
        if (sample_queue_index == APU_BUFFER_SIZE)
            sample_queue_index = 0;
    }

    sample_timer++;

    global_timer++;
}