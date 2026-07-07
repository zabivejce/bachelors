#include <algorithm>
#include <asm-generic/socket.h>
#include <cmath>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <cstdint>
#include <memory>
#include <mutex>
#include <ostream>
#include <queue>
#include <string>
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
    test stations (these are close enough)
    100.5
    101.1
    101.4
*/
std::vector<std::shared_ptr<RadioWorker>> workers;
std::mutex workersMutex;

bool sendFullString(int sock, const std::string& data) 
{
    size_t total_sent = 0;
    const char* buf = data.c_str();
    size_t size_data = data.length();

    while (total_sent < size_data) 
    {
        ssize_t n_bytes = send(sock, buf + total_sent, size_data - total_sent, MSG_NOSIGNAL);
        
        if (n_bytes <= 0) 
        {
            return false;// connection error
        }
        total_sent += n_bytes;
    }
    return true;
}

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
        std::cerr << "ERROR: port 8080 is reserved."<< std::endl;
        std::exit(EXIT_FAILURE);
    }

    if(listen(srv_sock, 5) < 0)
    {
        std::cerr << "ERROR: listen failed. Cannot create queue for connection." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Main API runs on port " << port << " (VLC example: http://127.0.0.1:" << port << "/101.1)" << std::endl;

    while(true)
    {
        struct sockaddr_in cli_addr;
        socklen_t len = sizeof(cli_addr);
        int cli_sock = accept(srv_sock, (struct sockaddr*)&cli_addr, &len);
        if (cli_sock < 0) continue;

        std::string request;
        char buffer[1024] = {0};
        bool header_complete = false;

        while(!header_complete && request.length() < 8192) // safety size 8kiB
        {
            int bytes_read = read(cli_sock, buffer, sizeof(buffer) - 1);

            if(bytes_read > 0)
            {
                buffer[bytes_read] = '\0';
                request += buffer;

                if(request.find("\r\n\r\n") != std::string::npos)
                    header_complete = true;
            }
            else
            {
                break; // connection error
            }
        }

        if(!header_complete)
        {
            std::string err_resp = "HTTP/1.1 400 Bad Request\r\n\r\n";
            sendFullString(cli_sock,err_resp);
            close(cli_sock);
            continue;
        }

        if(request.substr(0,4) != "GET ")
        {
            std::string err_resp = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            sendFullString(cli_sock, err_resp);
            close(cli_sock);
            continue;
        }
        
        std::string host_ip = "127.0.0.1";//fallback
        size_t host_pos = request.find("Host: ");
        if(host_pos == std::string::npos) host_pos = request.find("host: ");
        
        if(host_pos != std::string::npos)
        {
            host_pos += 6; // "Host: " skip
            size_t host_end = request.find("\r\n", host_pos);
            if(host_end != std::string::npos)
            {
                std::string full_host = request.substr(host_pos, host_end - host_pos);
                
                size_t colon_pos = full_host.find(":");
                if(colon_pos != std::string::npos)
                {
                    host_ip = full_host.substr(0, colon_pos);
                }
                else
                {
                    host_ip = full_host;
                }
            }
        }
        
        size_t first_line_end = request.find("\r\n");
        if (first_line_end != std::string::npos)
        {
            std::string first_line = request.substr(0, first_line_end);
            
            if(first_line.find("GET /") == 0)
            {
                size_t start = 5; 
                size_t end = first_line.find(" ", start); 
                
                if(end != std::string::npos && end > start)
                {
                    std::string freq_str = first_line.substr(start, end - start);
                    
                    try
                    {
                        float target_freq = std::stof(freq_str);
                        float offset_hz = (target_freq - SDRParams::sdr_center_freq) * 1000000.0f;

                        if(offset_hz < -(SDRParams::sdr_rate / 2.0f) || offset_hz > (SDRParams::sdr_rate / 2.0f)){throw std::out_of_range("Out of range of RTL-SDR tuner");}
                        
                        auto worker = std::make_shared<RadioWorker>(offset_hz);
                        int stream_port = worker->initNetwork();
                        if(stream_port <= 0)
                        {
                            std::string err_resp = "HTTP/1.1 500 Internal Server Error\r\n\r\n";
                            sendFullString(cli_sock, err_resp);
                            close(cli_sock);
                            continue;
                        }
                        worker->start();
                        {
                            std::lock_guard<std::mutex> lock(workersMutex);
                            workers.push_back(worker);
                        }
                        //redirect to different port
                        //https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status/302
                        std::string resp = "HTTP/1.1 302 Found\r\nLocation: http://" + host_ip + ":" + std::to_string(stream_port) + "\r\nConnection: close\r\n\r\n";
                        sendFullString(cli_sock, resp);
                        //write(cli_sock, resp.c_str(), resp.length());
                    }
                    //Out of range catch
                    //https://developer.mozilla.org/en-US/docs/Web/HTTP/Reference/Status/416
                    catch(const std::out_of_range& e)
                    {
                        std::cout << "Failed: " << e.what() << std::endl;
                        std::string resp = "HTTP/1.1 416 Range Not Satisfiable\r\n\r\n";
                        sendFullString(cli_sock,resp);
                        //write(cli_sock, resp.c_str(), resp.length());
                        
                    }
                    //other catch
                    catch(...)
                    {
                        std::string resp = "HTTP/1.1 400 Bad Request\r\n\r\n";
                        sendFullString(cli_sock, resp);
                        //write(cli_sock, resp.c_str(), resp.length());
                    }
                }
            }
        }
        close(cli_sock);
    }
}

void help(const char* prog)
{
    std::cerr << "Usage: " << prog << " <center_frequency>\n\n"
              << "Example:\n"
              << "\t" << prog << " 100.0\n"
              << "\t" << prog << " 101.1\n\n"
              << "Description:\n"
              << "\tThis parameter defines the center frequency in MHz tuned on the RTL-SDR hardware.\n"
              << "\tThe program expects I/Q data on the standard input (stdin).\n"
              << "\tThe program expects 2.4 MS/s.\n\n"
              << "Complete example using a pipe:\n"
              << "\trtl_sdr -f 100.0M -s 2.4M - | " << prog << " 100.0\n";
}

int main(int argc, char* argv[])
{
    if(argc != 2 || std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")
    {
        help(argv[0]);
        return 1;
    }

    try
    {
        SDRParams::sdr_center_freq = std::stof(argv[1]);
        std::cout << "[INFO] Center frequency set to: " << SDRParams::sdr_center_freq << " MHz\n";
    }
    catch(const std::exception& e)
    {
        std::cerr << "[ERROR] Invalid frequency format. Please use format like 100.0 or 101.1\n";
        return 1;
    }

    SDRParams::sdr_rate = 2400000;
    SDRParams::audio_rate = 48000;
    SDRParams::fir_cutoff = 100000;
    SDRParams::gain = 10000;
    std::thread httpThread(httpServerLoop, 8080);
    httpThread.detach();

    const int SAMPLES_PER_CHUNK = 1024;
    uint8_t raw_buffer[SAMPLES_PER_CHUNK * 2]; 

    std::cout << "Core system started. Feed IQ data to stdin..." << std::endl;

    //distribution of IQ blocks
    while(std::cin.read(reinterpret_cast<char*>(raw_buffer), sizeof(raw_buffer)))
    {
        auto block = std::make_shared<IQBlock>();

        for(int i = 0; i < SAMPLES_PER_CHUNK; ++i)
        {
            float i_val = ((float)raw_buffer[2*i] - 127.5f) / 127.5f;
            float q_val = ((float)raw_buffer[2*i+1] - 127.5f) / 127.5f;
            block->i_data.push_back(i_val);
            block->q_data.push_back(q_val);
        }

        {
            std::lock_guard<std::mutex> lock(workersMutex);
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