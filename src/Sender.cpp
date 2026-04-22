#include "Sender.hpp"
bool Sender::startServerAccept()
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

    // === PŘIDANÝ KÓD PRO WAV HLAVIČKU ===
    uint32_t sample_rate = static_cast<uint32_t>(SDRParams::audio_rate);
    uint16_t channels = 1;         // Mono (podle Channel.cpp)
    uint16_t bits_per_sample = 16; // int16_t (podle Channel.cpp)

    uint8_t wav_header[44];
    std::memcpy(wav_header, "RIFF", 4);
    uint32_t file_size = 0xFFFFFFFF; // Velikost streamu (neznámá/nekonečná)
    std::memcpy(wav_header + 4, &file_size, 4);
    std::memcpy(wav_header + 8, "WAVE", 4);
    std::memcpy(wav_header + 12, "fmt ", 4);
    uint32_t fmt_size = 16;          // Velikost fmt bloku pro PCM
    std::memcpy(wav_header + 16, &fmt_size, 4);
    uint16_t format = 1;             // 1 = nekomprimované PCM
    std::memcpy(wav_header + 20, &format, 2);
    std::memcpy(wav_header + 22, &channels, 2);
    std::memcpy(wav_header + 24, &sample_rate, 4);
    uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
    std::memcpy(wav_header + 28, &byte_rate, 4);
    uint16_t block_align = channels * (bits_per_sample / 8);
    std::memcpy(wav_header + 32, &block_align, 2);
    std::memcpy(wav_header + 34, &bits_per_sample, 2);
    std::memcpy(wav_header + 36, "data", 4);
    uint32_t data_size = 0xFFFFFFFF; // Velikost dat (neznámá/nekonečná)
    std::memcpy(wav_header + 40, &data_size, 4);

    // Odeslání hlavičky ihned po úspěšném navázání spojení
    //send(conn_sock, wav_header, 44, MSG_NOSIGNAL);
    // ====================================

    return true;
}
bool Sender::sendData(void* data, long size_data)
{
    if(conn_sock < 0)
        return false;
    //ssize_t muze ulozit i -1 [more you know :)]
    //ssize_t n_bytes = write(conn_sock, data, size_data);
    ssize_t n_bytes = send(conn_sock, data, size_data, MSG_NOSIGNAL);
    sendCnt += size_data;
    if(sendCnt >= 480000)
    {
        sendCnt = 0;
        auto now = std::chrono::system_clock::now();
        time_t unix_time = std::chrono::system_clock::to_time_t(now);

        std::cout << "cas: " << unix_time << std::endl;
    }
    if(n_bytes != size_data)
    {
        std::cout << "client disconnected" << std::endl;
        close(conn_sock);
        return false;
    }
    return true;
}