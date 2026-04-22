#pragma once
#include "Sender.hpp"
#include "FIRFilter.hpp"
#include "SDRParams.hpp"
#include <cmath>
#include <algorithm>
#include <vector>
class Channel{
    private:
        float freq_offset;
        float phase;
        float phase_inc;
        float last_i;
        float last_q;
        float i;
        float q;
        float last_phase;
        float lpf_i;
        float lpf_q;
        float lpf_alpha;

        int decim_cnt;
        float decim_tmp;
        int decimation;
        float sum_i, sum_q;

        Sender* sndr;
        FIRFilter* filter;

        std::vector<int16_t> audio_buffer;

        void mixFreq();
        float demod(float i_dem, float q_dem);
        bool decim(float demod, float& audio);
    public:
        Channel(float ch_offset, Sender* sender)
        {
            std::cout << "channel start\n";
            freq_offset = ch_offset;
            phase_inc = 2.0f * M_PI * freq_offset / SDRParams::sdr_rate;
            lpf_alpha = 0.15f;
            lpf_i = 0.0f;
            lpf_q = 0.0f;
            phase = 0.0f;
            last_phase = 0.0f;
            decim_cnt = 0;
            decim_tmp = 0.0f;
            i = 0.0f; q = 0.0f;
            sndr = sender;
            std::cout << "before fir\n";
            filter = new FIRFilter();
            std::cout << "after fir\n";
            decimation = (int)(SDRParams::sdr_rate / SDRParams::audio_rate);
            std::cout << decimation << std::endl;
            audio_buffer.reserve(512);
        }
        bool process(float i_in,float q_in);
};