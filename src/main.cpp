#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstdint>
#include <vector>
/*
https://k3xec.com/packrat-processing-iq/
https://htorp.folk.ntnu.no/Undervisning/TTK10/IQdemodulation.pdf
https://en.wikipedia.org/wiki/In-phase_and_quadrature_components
*/
/*
multithread
data pujdou pres pipe
kazde vlakno bude mit svoji instanci Channel
bude nasledne dalsi class na Sender se svojim socketem, nebo bude vsechno implementovane v class Channel? (nejspise bude class Sender)

*/

const int sdr_rate = 2400000;
const int audio_rate = 48000;
const int decimation = sdr_rate / audio_rate;
const int gain = 16000;
const float freq_offset = 100000.0f;

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

        int decim_cnt;
        float decim_tmp;
    public:
        Channel(float ch_offset)
        {
            freq_offset = ch_offset;
            phase_inc = 2.0f * M_PI * freq_offset / sdr_rate;
        }
        void setIQ(float i, float q)
        {
            this->i = i;
            this->q = q;
        }
        void mixFreq()
        {
            float cosinus = std::cos(-phase);
            float sinus = std::sin(-phase);
            float new_i = i * cosinus - q * sinus;
            float new_q = i * sinus + q * cosinus;
            i = new_i;
            q = new_q;
        }
        float demod()
        {
            //polar discriminant
            float current_phase = std::atan2(q, i);
            float delta = current_phase - last_phase;

            // unwrap phase difference
            if (delta > M_PI) delta -= 2.0f * M_PI;
            if (delta < -M_PI) delta += 2.0f * M_PI;

            last_phase = current_phase;
            return delta;

            /*//Conjugate
            float dI = last_i * i + last_q * q;   // Re[s[n] * conj(s[n-1])]
            float dQ = last_i * q - last_q * i;   // Im[s[n] * conj(s[n-1])]
            float delta = std::atan2(dQ, dI);     // instantaneous phase difference

            last_i = i;
            last_q = q;
            return delta;
            */

            /*
            float ret = (last_i * q - last_q * i);
            last_i = i;
            last_q = q;
            return ret;
            */
        }
        bool decim(float demod, float& audio)
        {
            decim_tmp += demod;
            ++decim_cnt;
            if(decimation == decim_cnt)
            {
                audio = decim_tmp / decimation;
                decim_cnt = 0;
                decim_tmp = 0.0;
                return true;
            }
            return false;
        }
};


int main()
{
    uint8_t iq[2];
    std::vector<Channel*> channels;
    channels[0] = new Channel(0.0f);
    float i, q;
    float demod, audio;

    while(std::cin.read(reinterpret_cast<char*>(iq),2))
    {
        i = ((float)iq[0] - 127.5f) / 127.5f;
        q = ((float)iq[1] - 127.5f) / 127.5f;

        channels[0]->setIQ(i,q);
        channels[0]->mixFreq();
        demod = channels[0]->demod();
        if(channels[0]->decim(demod,audio))
        {
            float scaled = audio * (float)gain;
            int16_t sample = static_cast<int16_t>(std::clamp(scaled, -32767.0f, 32767.0f));
            std::cout.write(reinterpret_cast<char*>(&sample), sizeof(sample));
        }
    }
    return 0;
}