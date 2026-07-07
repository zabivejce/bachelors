#pragma once
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>
#include "SDRParams.hpp"
#include "IQBlock.hpp"
#include "Sender.hpp"
#include "Channel.hpp"
class RadioWorker
{
    private:
        int port;
        int offset;
        std::atomic<bool> running{true}; // resolved issue with data race
        std::unique_ptr<Sender> sender;
        std::unique_ptr<Channel> channel;
        std::thread workerThread;

        std::queue<std::shared_ptr<IQBlock>> dataQueue;
        std::mutex queueMutex;
        std::condition_variable queueCv;

        const size_t MAX_QUEUE_SIZE = 256; 

        void threatLoop();
    public:
        RadioWorker(float offset) : offset(offset), running(true)
        {
            sender = std::make_unique<Sender>();
            channel = std::make_unique<Channel>(offset,sender.get());
        }
        bool isRunning() {return running;}
        void start() {workerThread = std::thread(&RadioWorker::threatLoop, this);}
        void stop();
        void pushData(const std::shared_ptr<IQBlock>& block);
        int initNetwork()
        {
            port = sender->initServer();
            return port;
        }
};