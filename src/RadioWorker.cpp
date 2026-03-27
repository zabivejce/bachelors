#include "RadioWorker.hpp"
void RadioWorker::threatLoop()
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
void RadioWorker::stop()
{
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        running = false;
    }
    queueCv.notify_all();

    if(workerThread.joinable())
        workerThread.join();
}
void RadioWorker::pushData(const std::shared_ptr<IQBlock>& block)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    if(!running)
        return;

    if(dataQueue.size() >= MAX_QUEUE_SIZE)
        dataQueue.pop();

    dataQueue.push(block);
    queueCv.notify_one();
}