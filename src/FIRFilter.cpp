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

    if(head == 0) 
        head = num_taps - 1;
    else
        head = head - 1;

    float out_i = 0.0f;
    float out_q = 0.0f;
    const float* p_i = &buffer_i[head + 1];
    const float* p_q = &buffer_q[head + 1];
    const float* p_taps = taps.data();

    //#pragma omp simd reduction(+:out_i, out_q)
    #pragma omp simd reduction(+:out_i, out_q)
    for(int k = 0; k < num_taps; ++k)
    {
        out_i += p_i[k] * p_taps[k];
        out_q += p_q[k] * p_taps[k];
    }

    i = out_i;
    q = out_q;
}

void FIRFilter::process(float &audio)
{
    buffer_audio[head] = audio;
    buffer_audio[head + num_taps] = audio;

    if(head == 0)
        head = num_taps - 1;
    else
        head = head - 1;

    float out_audio = 0.0f;
    const float* p_a = &buffer_audio[head + 1];
    const float* p_taps = taps.data();

    #pragma omp simd reduction(+:out_audio)
    for(int k = 0; k < num_taps; ++k)
    {
        out_audio += p_a[k] * p_taps[k];
    }

    audio = out_audio;
}

void FIRFilter::generateFirTaps(float cutoff, float rate)
{
    //Normalizovana orezova frekvence
    float normalized_cutoff = cutoff / rate;
    float sum = 0.0f;
    int M = num_taps - 1;
    taps.resize(num_taps); 

    for(int i = 0; i < num_taps; ++i)
    {
        //Vzdalenost od stredu filtru
        float n = i - M / 2.0f;

        if(n == 0.0f)
        {
            //Stred filtru: Vyhneme se deleni nulou v Sinc funkci
            taps[i] = 2.0f * normalized_cutoff;
        }
        else
        {
            //Vypocet Sinc funkce pro dolni propust
            float x = 2.0f * M_PI * normalized_cutoff * n;
            taps[i] = std::sin(x) / (M_PI * n);
        }

        //Aplikace Hammingova okna
        float window = 0.54f - 0.46f * std::cos((2.0f * M_PI * i) / M);
        taps[i] *= window;

        //Prubezne scitame pro naslednou normalizaci
        sum += taps[i];
    }

    //Normalizace koeficientu
    for(int i = 0; i < num_taps; ++i)
        taps[i] /= sum;

    //debug
    //for(auto t : taps)
    //    std::cout << t << ", ";
    //std::cout << std::endl;
}
