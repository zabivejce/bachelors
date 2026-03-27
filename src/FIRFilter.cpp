#include "FIRFilter.hpp"
#include "SDRParams.hpp"
#include <cmath>
#include <iostream>
void FIRFilter::process(float &i, float &q)
{
    buffer_i[head] = i;
    buffer_i[head + num_taps] = i;
    buffer_q[head] = q;
    buffer_q[head + num_taps] = q;

    //head = (head == 0) ? (num_taps - 1) : (head - 1);
    if(head == 0) 
        head = num_taps - 1;
    else
        head = head - 1;

    float out_i = 0.0f;
    float out_q = 0.0f;
    const float* p_i = &buffer_i[head + 1];
    const float* p_q = &buffer_q[head + 1];
    const float* p_taps = taps.data();

    #pragma omp simd
    for (int k = 0; k < num_taps; ++k)
    {
        out_i += p_i[k] * p_taps[k];
        out_q += p_q[k] * p_taps[k];
    }

    //++head;
    //if (head >= num_taps) head = 0;

    i = out_i;
    q = out_q;
}

void FIRFilter::generateFirTaps()
{
    // Normalizovaná ořezová frekvence (vzhledem k vzorkovací frekvenci)
    float normalized_cutoff = SDRParams::fir_cutoff / SDRParams::sdr_rate;
    float sum = 0.0f;
    int M = num_taps - 1; // Řád filtru
    taps.resize(num_taps); 

    for(int i = 0; i < num_taps; ++i)
    {
        // Vzdálenost od středu filtru
        float n = i - M / 2.0f;

        if(n == 0.0f)
        {
            // Střed filtru: Vyhneme se dělení nulou v Sinc funkci
            taps[i] = 2.0f * normalized_cutoff;
        }
        else
        {
            // 1. Krok: Výpočet Sinc funkce pro dolní propust
            float x = 2.0f * M_PI * normalized_cutoff * n;
            taps[i] = std::sin(x) / (M_PI * n);
        }

        // 2. Krok: Aplikace Hammingova okna (zjemnění krajů filtru)
        float window = 0.54f - 0.46f * std::cos((2.0f * M_PI * i) / M);
        taps[i] *= window;

        // Průběžně sčítáme pro následnou normalizaci
        sum += taps[i];
    }

    // 3. Krok: Normalizace koeficientů (zisk na DC = 0 Hz bude přesně 1.0)
    for(int i = 0; i < num_taps; ++i)
    {
        taps[i] /= sum;
    }

    for(auto t : taps)
    {
        std::cout << t << ", ";
    }
    std::cout << std::endl;
}
