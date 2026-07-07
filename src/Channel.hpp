#pragma once
#include "Sender.hpp"
#include "FIRFilter.hpp"
#include "SDRParams.hpp"
#include <cmath>
#include <algorithm>
#include <memory>
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

        int rf_decim_cnt;
        int af_decim_cnt;
        float af_decim_buff;
        int decimation;

        float deemph_y = 0.0f;

        Sender* sndr;
        std::unique_ptr<FIRFilter> IQfilter;
        std::unique_ptr<FIRFilter> audio_filter;

        std::vector<int16_t> audio_buffer;

        void mixFreq();
        float demod(float i_dem, float q_dem);
        bool decim(float demod, float& audio);
    public:
        Channel(float ch_offset, Sender* sender)
        {
            freq_offset = ch_offset;
            phase_inc = 2.0f * M_PI * freq_offset / SDRParams::sdr_rate;
            lpf_alpha = 0.15f;
            lpf_i = 0.0f;
            lpf_q = 0.0f;
            phase = 0.0f;
            last_phase = 0.0f;
            rf_decim_cnt = 0;
            af_decim_cnt = 0;
            af_decim_buff = 0.0f;
            i = 0.0f; q = 0.0f;
            sndr = sender;
            IQfilter = std::make_unique<FIRFilter>(SDRParams::fir_cutoff, SDRParams::sdr_rate);
            audio_filter = std::make_unique<FIRFilter>(15000.0f, 240000.0f);// cutoff 15kHz, cut 19kHz pilot and above
            decimation = (int)(SDRParams::sdr_rate / SDRParams::audio_rate);
            audio_buffer.reserve(512);
        }
        bool process(float i_in,float q_in);
};