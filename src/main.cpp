#include <algorithm>
#include <ctime>
#include <iostream>
#include <cstdint>
/*
https://k3xec.com/packrat-processing-iq/
https://htorp.folk.ntnu.no/Undervisning/TTK10/IQdemodulation.pdf
https://en.wikipedia.org/wiki/In-phase_and_quadrature_components
*/

const int sdr_rate = 2400000;
const int audio_rate = 48000;
const int decimation = sdr_rate / audio_rate;
const int gain = 16000;

int count = 0;
float tmp = 0.0f;
bool lpf_dec(float x, float& y)
{
    tmp += x;
    ++count;
    if(count == decimation)
    {
        y = tmp / decimation;
        tmp = 0.0f;
        count = 0;
        return true;
    }
    return false;
}
int main()
{
    uint8_t iq[2];
    float i,q,last_i,last_q;
    float demod,audio;
    while(std::cin.read(reinterpret_cast<char*>(iq),2))
    {
        i = ((float)iq[0] - 127.5f) / 127.5f;
        q = ((float)iq[1] - 127.5f) / 127.5f;

        demod = (last_i * q - last_q * i);
        if(lpf_dec(demod, audio))
        {
            float scaled = audio * (float)gain;
            int16_t sample = static_cast<int16_t>(std::clamp(scaled, -32767.0f, 32767.0f));
            std::cout.write(reinterpret_cast<char*>(&sample), sizeof(sample));
        }

        last_i = i;
        last_q = q;
    }
    return 0;
}