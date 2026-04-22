#pragma once
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include "SDRParams.hpp"
#include <chrono>
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
        ~Sender()
        {
            if(conn_sock >= 0)
                close(conn_sock);
            if(listen_sock >= 0)
                close(listen_sock);
        }
        bool startServerAccept();
        bool sendData(void* data, long size_data);
};