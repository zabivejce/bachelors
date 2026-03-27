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

#include "IQBlock.hpp"
#include "Channel.hpp"
#include "SDRParams.hpp"
#include "FIRFilter.hpp"
#include "Sender.hpp"
#include "RadioWorker.hpp"
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

//const int sdr_rate = 2048000;
//const int audio_rate = 48000;
//const int decimation = sdr_rate / audio_rate;
//const int gain = 32000;

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
                
                response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nWorker started.\n";
            }
        }
        
        write(cli_sock, response.c_str(), response.length());
        close(cli_sock);
    }
    close(srv_sock);
}

int main()
{
    SDRParams::sdr_rate = 2400000;
    SDRParams::gain = 32000;
    SDRParams::audio_rate = 48000;
    SDRParams::fir_cutoff = 100000;
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