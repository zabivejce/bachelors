#pragma once
#include <vector>
#include <cmath>
#include <iostream>
#include "SDRParams.hpp"
class FIRFilter
{
    private:
        std::vector<float> taps;
        std::vector<float> buffer_i;//Historie I vzorku
        std::vector<float> buffer_q;//Historie Q vzorku
        std::vector<float> buffer_audio;

        int head;//Ukazatel na nejnovejsi vzorek
        const int num_taps = 63;//Pocet koeficientu

        void generateFirTaps(float cutoff, float rate);
    public:
        FIRFilter(float cutoff, float rate)
        {
            generateFirTaps(cutoff, rate);
            buffer_i.assign(num_taps * 2,0.0f);
            buffer_q.assign(num_taps * 2,0.0f);

            buffer_audio.assign(num_taps * 2, 0.0f);

            head = 0;
        }
        void process(float &i, float &q); //for IQ
        void process(float &audio); //for audio to remove pilot and higher
};