#pragma once
#include <atomic>
#include <sys/socket.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include "SDRParams.hpp"
class Sender{
    private:
        int srv_sock;
        int conn_sock;
        int ass_port;
    public:
        Sender()
        {
            srv_sock = -1;
            conn_sock = -1;
            ass_port = -1;
        }
        ~Sender()
        {
            if(conn_sock >= 0)
                close(conn_sock);
            if(srv_sock >= 0)
                close(srv_sock);
        }
        int initServer();
        bool startServerAccept(std::atomic<bool>& isRunning);
        bool sendData(void* data, long size_data);
};