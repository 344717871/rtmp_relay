#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H

#include <boost/thread.hpp>  
#include <boost/thread/mutex.hpp>
#include <queue>

typedef struct{
    unsigned char* _pData;
    int _iLength;
}DATA_QUEUE_ITEM;

class DataQueue
{
public:
    DataQueue();
    ~DataQueue();

    int InsertQueue(unsigned char* pData, int iLength);
    DATA_QUEUE_ITEM* GetAndReleaseQueue();
    void ClearDataQueue();
    int GetQueueLength();
private:
    std::queue<DATA_QUEUE_ITEM> _dataQueue;
    boost::mutex _queueMutex;  
};
#endif//DATA_QUEUE_H