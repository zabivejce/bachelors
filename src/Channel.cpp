#include "Channel.hpp"
#include <cstdint>

void Channel::mixFreq()
{
    float cosinus = std::cos(-phase);
    float sinus = std::sin(-phase);
    float new_i = i * cosinus - q * sinus;
    float new_q = i * sinus + q * cosinus;
    i = new_i;
    q = new_q;

    phase += phase_inc;
    if(phase > 2.0f * M_PI)
        phase -= 2.0f * M_PI;
    if(phase < -2.0f * M_PI)
        phase += 2.0f * M_PI;
}
float Channel::demod()
{
    //polar discriminant
    float current_phase = std::atan2(q, i);
    float delta = current_phase - last_phase;

    // unwrap phase difference
    if (delta > M_PI) delta -= 2.0f * M_PI;
    else if (delta < -M_PI) delta += 2.0f * M_PI;

    last_phase = current_phase;
    return delta;
}
bool Channel::decim(float demod, float& audio)
{
    decim_tmp += demod;
    ++decim_cnt;
    if(decimation <= decim_cnt)
    {
        audio = decim_tmp / decimation;
        decim_cnt = 0;
        decim_tmp = 0.0;
        return true;
    }
    return false;
}
/*
void filterIQ()
{
    i = lpf_i + lpf_alpha * (i - lpf_i);
    q = lpf_q + lpf_alpha * (q - lpf_q);
    lpf_i = i;
    lpf_q = q;
}
*/
bool Channel::process(float i_in,float q_in)
{
    i = i_in;
    q = q_in;
    mixFreq();
    filter->process(i, q);
    float audio;
    float scaled;

    if(decim(demod(), audio))
    {
        scaled = audio * SDRParams::gain;
        int16_t sample = static_cast<int16_t>(std::clamp(scaled, -32767.0f, 32767.0f));
        audio_buffer.emplace_back(sample);
        if(sndr && audio_buffer.size() >= 512)
        {
            bool status = sndr->sendData(audio_buffer.data(),audio_buffer.size() * sizeof(int16_t));
            audio_buffer.clear();
            return status;
        }
    }
    return true;
}