#include <algorithm>
#include <asm-generic/socket.h>
#include <cmath>
#include <condition_variable>
#include <iostream>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
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
const int CHUNK_SIZE = 1024;


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
            listen_sock = -1;

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
            //ssize_t n_bytes = write(conn_sock, data, size_data);
            ssize_t n_bytes = send(conn_sock, data, size_data, MSG_NOSIGNAL);

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

struct IQBlock
{
    std::vector<float> i_data;
    std::vector<float> q_data;
    IQBlock()
    {
        i_data.reserve(CHUNK_SIZE);
        q_data.reserve(CHUNK_SIZE);
    }
};

class RadioWorker
{
    private:
        int port;
        int offset;
        bool running;
        std::unique_ptr<Sender> sender;
        std::unique_ptr<Channel> channel;
        std::thread workerThread;

        std::queue<std::shared_ptr<IQBlock>> dataQueue;
        std::mutex queueMutex;
        std::condition_variable queueCv;

        const size_t MAX_QUEUE_SIZE = 50; 

        void threatLoop()
        {
            sender = std::make_unique<Sender>(port);
            if(!sender->startServerAccept())
            {
                running = false;
                return;
            }

            channel = std::make_unique<Channel>(offset, sender.get());

            while(running)
            {
                std::shared_ptr<IQBlock> block;
                { // zamek zanikne po ukonceni scope
                    std::unique_lock<std::mutex> lock(queueMutex);
                    // cekani na data
                    queueCv.wait(lock, [this] {return !dataQueue.empty() || !running;});
                    if(!running && dataQueue.empty())
                        break;
                
                    if(!dataQueue.empty())
                    {
                        block = dataQueue.front();
                        dataQueue.pop();
                    }
                    if(block)
                    {
                        for(size_t k = 0; k < block->i_data.size(); ++k)
                        {
                            // klient se odpojil
                            if(!channel->process(block->i_data[k], block->q_data[k]))
                            {
                                std::cout << "[Worker " << port << "] Processing stopped (client disconnected)." << std::endl;
                                running = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

    public:
        RadioWorker(int port, float offset) : port(port), offset(offset), running(true)
        {}
        bool isRunning()
        {return running;}
        void start()
        {workerThread = std::thread(&RadioWorker::threatLoop, this);}
        void stop()
        {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                running = false;
            }
            queueCv.notify_all();

            if(workerThread.joinable())
                workerThread.join();
        }
        void pushData(const std::shared_ptr<IQBlock>& block)
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if(!running)
                return;

            if(dataQueue.size() >= MAX_QUEUE_SIZE)
                dataQueue.pop();

            dataQueue.push(block);
            queueCv.notify_one();
        }
};

std::vector<std::shared_ptr<RadioWorker>> workers;
std::mutex workersMutex;

void httpServerLoop(int port)
{
    int srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(srv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    if(bind(srv_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "HTTP Server: Bind failed on port " << port << std::endl;
        return;
    }
    listen(srv_sock, 5);
    std::cout << "HTTP Server listening on port " << port << std::endl;

    while(true)
    {
        struct sockaddr_in cli_addr;
        socklen_t len = sizeof(cli_addr);
        int cli_sock = accept(srv_sock, (struct sockaddr*)&cli_addr, &len);
        if (cli_sock < 0) continue;

        char buffer[1024] = {0};
        read(cli_sock, buffer, 1024);
        std::string request(buffer);
        
        std::string response = "HTTP/1.1 400 Bad Request\r\n\r\n";
        
        // parsing: GET /start?port=1234&offset=50000
        if(request.find("GET") == 0)
        {
            int new_port = 0;
            float new_offset = 0.0f;
            
            size_t pos_port = request.find("port=");
            size_t pos_off = request.find("offset=");
            
            if(pos_port != std::string::npos && pos_off != std::string::npos)
            {
                new_port = std::stoi(request.substr(pos_port + 5));
                new_offset = std::stof(request.substr(pos_off + 7));
                
                std::cout << "HTTP: Spawning worker on port " << new_port << ", offset " << new_offset << std::endl;
                
                auto worker = std::make_shared<RadioWorker>(new_port, new_offset);
                worker->start();
                
                {
                    std::lock_guard<std::mutex> lock(workersMutex);
                    workers.push_back(worker);
                }
                
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nWorker started.";
            }
        }
        
        write(cli_sock, response.c_str(), response.length());
        close(cli_sock);
    }
    close(srv_sock);
}

int main()
{
    std::thread httpThread(httpServerLoop, 8080);
    httpThread.detach();

    const int SAMPLES_PER_CHUNK = 1024;
    uint8_t raw_buffer[SAMPLES_PER_CHUNK * 2]; 

    std::cout << "Core system started. Feed IQ data to stdin..." << std::endl;
    std::cout << "Use HTTP to spawn workers: curl 'http://localhost:8080/start?port=8008&offset=-200000'" << std::endl;

    while(std::cin.read(reinterpret_cast<char*>(raw_buffer), sizeof(raw_buffer)))
    {
        auto block = std::make_shared<IQBlock>();

        // mohlo by byt v intech a ne ve floatech :)
        for(int i = 0; i < SAMPLES_PER_CHUNK; ++i)
        {
            float i_val = ((float)raw_buffer[2*i] - 127.5f) / 127.5f;
            float q_val = ((float)raw_buffer[2*i+1] - 127.5f) / 127.5f;
            block->i_data.push_back(i_val);
            block->q_data.push_back(q_val);
        }

        {
            std::lock_guard<std::mutex> lock(workersMutex);
            // pres iteratory, protoze je potom mazu
            // producer -> consumer
            for(auto it = workers.begin(); it != workers.end();)
            {
                if((*it)->isRunning())
                {
                    (*it)->pushData(block);
                    ++it;
                }
                else
                {
                    (*it)->stop();
                    it = workers.erase(it);
                }
            }
        }
    }

    std::cout << "Input stream ended. Shutting down." << std::endl;
    {
        std::lock_guard<std::mutex> lock(workersMutex);
        for(auto& w : workers)
            w->stop();
    }
    
    return 0;
}