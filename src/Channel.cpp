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
bool Channel::process(float i_in,float q_in)
{
    i = i_in;
    q = q_in;
    mixFreq();
    IQfilter->process(i, q);
    ++rf_decim_cnt;

    if(rf_decim_cnt >= 10) // srazeni na 240kS/s
    {
        rf_decim_cnt = 0;
        float audio = demod(i, q);
        audio_filter->process(audio);
        ++af_decim_cnt;
        if(af_decim_cnt >= 5) // srazeni na 48kS/s
        {
            af_decim_cnt = 0;

            float scaled = audio * SDRParams::gain;
            int16_t sample = static_cast<int16_t>(std::clamp(scaled, -32767.0f, 32767.0f));
            audio_buffer.emplace_back(sample);

            if(sndr && audio_buffer.size() >= 512)
            {
                bool status = sndr->sendData(audio_buffer.data(),audio_buffer.size() * sizeof(int16_t));
                audio_buffer.clear();
                return status;
            }
        }
    }
    return true;
}