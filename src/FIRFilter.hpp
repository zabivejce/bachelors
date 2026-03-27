#pragma once
#include <vector>
#include <cmath>
#include <iostream>
#include "SDRParams.hpp"
class FIRFilter
{
    private:
        //std::vector<float> taps = {0.000795f, 0.000769f, 0.000725f, 0.000637f, 0.000465f, 0.000165f, -0.000303f, -0.000967f, -0.001834f, -0.002883f, -0.004055f, -0.005254f, -0.006346f, -0.007169f, -0.007535f, -0.007250f, -0.006130f, -0.004014f, -0.000785f, 0.003612f, 0.009164f, 0.015778f, 0.023280f, 0.031421f, 0.039888f, 0.048321f, 0.056333f, 0.063538f, 0.069575f, 0.074131f, 0.076965f, 0.077927f, 0.076965f, 0.074131f, 0.069575f, 0.063538f, 0.056333f, 0.048321f, 0.039888f, 0.031421f, 0.023280f, 0.015778f, 0.009164f, 0.003612f, -0.000785f, -0.004014f, -0.006130f, -0.007250f, -0.007535f, -0.007169f, -0.006346f, -0.005254f, -0.004055f, -0.002883f, -0.001834f, -0.000967f, -0.000303f, 0.000165f, 0.000465f, 0.000637f, 0.000725f, 0.000769f, 0.000795f};
        std::vector<float> taps;
        std::vector<float> buffer_i;   // Historie I vzorku
        std::vector<float> buffer_q;   // Historie Q vzorku
        void generateFirTaps();
        int head;                      // Ukazatel na nejnovejsi vzorek
        const int num_taps = 63;                  // Pocet koeficientu
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