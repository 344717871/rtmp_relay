#include "DataQueue.hpp"
#include <stdio.h>

#ifndef NULL
#define NULL 0
#endif

#define DATA_QUEUE_SIZE_MAX 100

DataQueue::DataQueue()
{
}

DataQueue::~DataQueue()
{
    ClearDataQueue();
}

int DataQueue::InsertQueue(unsigned char * pData,int iLength)
{
    int iCurrentSize = _dataQueue.size();
    if(iCurrentSize >= DATA_QUEUE_SIZE_MAX)
    {
        printf("InsertQueue: current queue size(%d) is overload.\r\n", iCurrentSize);
        ClearDataQueue();
    }
    DATA_QUEUE_ITEM item;
    item._pData = (unsigned char*)malloc(iLength);
    memcpy(item._pData, pData, iLength);
    item._iLength = iLength;

    _queueMutex.lock();
    _dataQueue.push(item);
    _queueMutex.unlock();
}

DATA_QUEUE_ITEM* DataQueue::GetAndReleaseQueue()
{
    DATA_QUEUE_ITEM* pRet = NULL;
    DATA_QUEUE_ITEM item;
    int iCurrentSize = _dataQueue.size();
    
    if(iCurrentSize <= 0)
    {
        return NULL;
    }
    _queueMutex.lock();
    item = _dataQueue.front();
    
    pRet = (DATA_QUEUE_ITEM*)malloc(sizeof(DATA_QUEUE_ITEM));
    pRet->_iLength = item._iLength;
    pRet->_pData   = item._pData;
    _dataQueue.pop();
    
    _queueMutex.unlock();
    return pRet;
}

int DataQueue::GetQueueLength()
{
    return _dataQueue.size();
}

void DataQueue::ClearDataQueue()
{
    DATA_QUEUE_ITEM item;
    
    _queueMutex.lock();
    while(_dataQueue.size() > 0)
    {
        item = _dataQueue.front();
        free(item._pData);
        _dataQueue.pop();
    }
    _queueMutex.unlock();
}