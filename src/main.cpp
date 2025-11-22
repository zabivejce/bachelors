#include <algorithm>
#include <asm-generic/socket.h>
#include <cmath>
#include <iostream>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/*
https://k3xec.com/packrat-processing-iq/
https://htorp.folk.ntnu.no/Undervisning/TTK10/IQdemodulation.pdf
https://en.wikipedia.org/wiki/In-phase_and_quadrature_components
*/
/*
na raw posilani dat do 
    rtl_sdr -f 101.1M -s 2.4M - | ./program | aplay -r 48000 -f S16_LE -c 1
*/
/*
hlavni vlakno hazi ostatnim vlaknu (workerum) IQ samply
Sender je primo im plementovany do Channel nebo je sam?
Sender bude promo ve vlakne s channelem (urcite)
po ukonceni poslechu se vlakno ukonci
OTAZKY:
    jak se bude spawnovat nove vlakno?
        hlavni vlakno bude na portu 80 a bude prijimat HTTP request?
        hlavni vlakno potom spawne nove vlakno s posunutim?
*/

const int sdr_rate = 2400000;
const int audio_rate = 48000;
const int decimation = sdr_rate / audio_rate;
const int gain = 16000;

class Sender{
    private:
        int listen_sock;
        int conn_sock;
        int port;
    public:
        Sender(int port)
        {
            listen_sock = -1;
            conn_sock = -1;
            this->port = port;
        }
        ~Sender() // pokud nejsou zavrene sockety, potom je zavru
        {
            if(conn_sock >= 0)
                close(conn_sock);
            if(listen_sock >= 0)
                close(listen_sock);
        }
        bool startServerAccept()
        {
            listen_sock = socket(AF_INET, SOCK_STREAM, 0);
            if(listen_sock < 0)
            {
                std::cerr << "ERR: cannot open listen socket." << std::endl;
                return false;
            }
            
            int opt = 1;
            setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in srv_addr;

            srv_addr.sin_family = AF_INET;
            srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
            srv_addr.sin_port = htons(port);

            if(bind(listen_sock, (struct sockaddr*)&srv_addr, sizeof(srv_addr)) < 0)
            {
                std::cerr << "ERR: bind err." << std::endl;
                close(listen_sock);
                return false;
            }

            listen(listen_sock,1);
            std::cout << "waiting on port " << port << std::endl;

            struct sockaddr_in cli_addr;
            socklen_t cli_len = sizeof(cli_addr);
            conn_sock = accept(listen_sock, (struct sockaddr*)&cli_addr, &cli_len);

            close(listen_sock);

            if(conn_sock < 0)
            {
                std::cerr << "ERR: accept failed" << std::endl;
                return false;
            }

            std::cout << "client connected" << std::endl;

            return true;
        }
        bool sendData(void* data, size_t size_data)
        {
            if(conn_sock < 0)
                return false;
            //ssize_t muze ulozit i -1 [more you know :)]
            ssize_t n_bytes = write(conn_sock, data, size_data);
            if(n_bytes != size_data)
            {
                std::cout << "client disconnected" << std::endl;
                close(conn_sock);
                return false;
            }
            return true;
        }

};

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
        float lpf_i;
        float lpf_q;
        float lpf_alpha;

        int decim_cnt;
        float decim_tmp;

        Sender* sndr;

        void mixFreq()
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
        void filterIQ()
        {
            i = lpf_i + lpf_alpha * (i - lpf_i);
            q = lpf_q + lpf_alpha * (q - lpf_q);
            lpf_i = i;
            lpf_q = q;
        }
    public:
        Channel(float ch_offset, Sender* sender)
        {
            freq_offset = ch_offset;
            phase_inc = 2.0f * M_PI * freq_offset / (float)sdr_rate;
            lpf_alpha = 0.15f;
            lpf_i = 0.0f;
            lpf_q = 0.0f;
            phase = 0.0f;
            last_phase = 0.0f;
            decim_cnt = 0;
            decim_tmp = 0.0f;
            i = 0.0f; q = 0.0f;
            sndr = sender;
        }
        bool process(float i_in,float q_in)
        {
            i = i_in;
            q = q_in;
            mixFreq();
            filterIQ();
            float audio;
            float scaled;

            if(decim(demod(), audio))
            {
                scaled = audio * (float)gain;
                int16_t sample = static_cast<int16_t>(std::clamp(scaled, -32767.0f, 32767.0f));
                if(sndr)
                {
                    return sndr->sendData(&sample,sizeof(sample));
                }
            }
            return true;
        }
};

void radioWorker(int port, float offset)
{}


int main()
{
    uint8_t iq[2];
    float i, q;
    float demod, audio;
    Sender* sndr0 = new Sender(8008);
    if(!sndr0->startServerAccept())
    {
        std::cerr << "server did not started" << std::endl;
        return 1;
    }
    Channel* chnl0 = new Channel(-200000.0f,sndr0);
    Sender* sndr1 = new Sender(8009);
    if(!sndr1->startServerAccept())
    {
        std::cerr << "server did not started" << std::endl;
        return 1;
    }
    Channel* chnl1 = new Channel(400000.0f,sndr1);

    while(std::cin.read(reinterpret_cast<char*>(iq),2))
    {
        i = ((float)iq[0] - 127.5f) / 127.5f;
        q = ((float)iq[1] - 127.5f) / 127.5f;
        if(!chnl0->process(i,q))
            break;
        if(!chnl1->process(i,q))
            break;
    }
    std::cout << "exiting program" << std::endl;
    return 0;
}