#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "IQBlock.hpp"
#include "Sender.hpp"
#include "Channel.hpp"
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

        void threatLoop();
    public:
        RadioWorker(int port, float offset) : port(port), offset(offset), running(true) {}
        bool isRunning() {return running;}
        void start() {workerThread = std::thread(&RadioWorker::threatLoop, this);}
        void stop();
        void pushData(const std::shared_ptr<IQBlock>& block);
};