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
        void generateFirTaps();
        int head;//Ukazatel na nejnovejsi vzorek
        const int num_taps = 63;//Pocet koeficientu
    public:
        FIRFilter()
        {
            generateFirTaps();
            buffer_i.assign(num_taps * 2,0.0f);
            buffer_q.assign(num_taps * 2,0.0f);
            head = 0;
        }
        void process(float &i, float &q);
};