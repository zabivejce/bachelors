#include "Sender.hpp"
#include <sys/socket.h>

int Sender::initServer()
{
    srv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(srv_sock < 0)
    {
        std::cerr << "[ERROR] Sender socket() failed" << std::endl;
        return -1;
    }

    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    addr.sin_port = htons(0); //OS will choose

    if(bind(srv_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        std::cerr << "[ERROR] Sender bind() failed" << std::endl;
        close(srv_sock);
        return -1;
    }

    if(listen(srv_sock, 5) < 0)
    {
        std::cerr << "[ERROR] Sender listen() failed" << std::endl;
        close(srv_sock);
        return -1;
    }

    socklen_t len = sizeof(addr);
    if(getsockname(srv_sock, (struct sockaddr*)&addr, &len) == -1)
    {
        std::cerr << "[ERROR] Sender getsockname() failed" << std::endl;
        close(srv_sock);
        return -1;
    }

    ass_port = ntohs(addr.sin_port);

    return ass_port;
}


bool Sender::startServerAccept(std::atomic<bool>& isRunning)
{
    struct pollfd pfd;
    pfd.fd = srv_sock;
    pfd.events = POLLIN;

    std::cout << "Worker listens on port " << ass_port << " ..." << std::endl;
    
    while(isRunning)
    {
        int poll_res = poll(&pfd, 1, 1000);// timeout 1s

        if(poll_res > 0 && (pfd.revents & POLLIN))
        {
            struct sockaddr_in cli_addr;
            socklen_t len = sizeof(cli_addr);

            conn_sock = accept(srv_sock, (struct sockaddr*)&cli_addr, &len);

            if(conn_sock >= 0)
            {
                std::cout << "Client connected on "<< ass_port << std::endl;
                const char* http_hdr = "HTTP/1.1 200 OK\r\nContent-Type: audio/wav\r\nConnection: close\r\n\r\n";
                sendData((void*)http_hdr, strlen(http_hdr));

                // === PRIDANY KOD PRO WAV HLAVICKU ===
                uint32_t sample_rate = static_cast<uint32_t>(SDRParams::audio_rate);
                uint16_t channels = 1;         // Mono (podle Channel.cpp)
                uint16_t bits_per_sample = 16; // int16_t (podle Channel.cpp)

                uint8_t wav_header[44];
                std::memcpy(wav_header, "RIFF", 4);
                uint32_t file_size = 0xFFFFFFFF;// Velikost streamu 4GiB vystaci na 12h prehravani
                std::memcpy(wav_header + 4, &file_size, 4);
                std::memcpy(wav_header + 8, "WAVE", 4);
                std::memcpy(wav_header + 12, "fmt ", 4);
                uint32_t fmt_size = 16;// Velikost fmt bloku pro PCM
                std::memcpy(wav_header + 16, &fmt_size, 4);
                uint16_t format = 1;// 1 = nekomprimovane PCM
                std::memcpy(wav_header + 20, &format, 2);
                std::memcpy(wav_header + 22, &channels, 2);
                std::memcpy(wav_header + 24, &sample_rate, 4);
                uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
                std::memcpy(wav_header + 28, &byte_rate, 4);
                uint16_t block_align = channels * (bits_per_sample / 8);
                std::memcpy(wav_header + 32, &block_align, 2);
                std::memcpy(wav_header + 34, &bits_per_sample, 2);
                std::memcpy(wav_header + 36, "data", 4);
                uint32_t data_size = 0xFFFFFFFF;
                std::memcpy(wav_header + 40, &data_size, 4);
                // ====================================
                sendData(wav_header, 44);

                return true;
            }
        }
        else if(poll_res == 0)
        {
            if(timeout_counter >= MAX_TIMEOUTS)
            {
                std::cout << "[WARNING] Client [" << ass_port << "] did not connected after " << MAX_TIMEOUTS << " tries. Releasing port." << std::endl;
                return false; // fail after 3 retrys
            }
            ++timeout_counter;
            continue; //no one connected, retry
        }
        else
        {
            break;
        }
    }

    return false;
}
bool Sender::sendData(void* data, long size_data)
{
    if(conn_sock < 0)
        return false;
    long total_sent = 0;
    char* buf = static_cast<char*>(data);
    while(total_sent < size_data)
    {
        ssize_t n_bytes = send(conn_sock, buf + total_sent, size_data - total_sent, MSG_NOSIGNAL);

        if(n_bytes <= 0)
        {
            std::cout << "client disconnected" << std::endl;
            close(conn_sock);
            conn_sock = -1;
            return false;
        }
        total_sent += n_bytes;
    }
    return true;
}