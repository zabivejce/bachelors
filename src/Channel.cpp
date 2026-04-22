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
float Channel::demod(float i_dem, float q_dem)
{
    //polar discriminant
    float current_phase = std::atan2(q_dem, i_dem);
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
bool Channel::process(float i_in,float q_in)
{
    i = i_in;
    q = q_in;
    mixFreq();
    filter->process(i, q);
    float audio;
    float scaled;
    ++decim_cnt;
    sum_i += i;
    sum_q += q;

    if(decim_cnt >= decimation)
    {
        audio = demod(sum_i, sum_q);
        decim_cnt = 0;
        sum_i = 0;
        sum_q = 0;
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