#ifndef RTMP_PUSH_H
#define RTMP_PUSH_H
#include "DataQueue.hpp"
#include "LibRtmpSession.hpp"
#include "FLVParser.hpp"

class RtmpPush
{
public:
    RtmpPush(char* szRtmpUrl);
    ~RtmpPush();

    int Start();
    void Stop();
    void OnWork();
private:
    RtmpPush();

    int waitForConnect();

    void DataHandle(unsigned char* pData, int iLength);

private:
    char _szRtmpUrl[512];
    unsigned char _pAscData[2];
    unsigned char _pPpsSpsData[512];
    
    LibRtmpSession* _rtmpSession;
    FLVParser* _flvParser;

    int _iStartFlag;
    pthread_t threadId;

    int _iThreadEndFlag;

    unsigned short _usASCFlag;

    int _iWidth;
    int _iHeigth;
    int _iFps;

    int _iConnect;
};

#endif//RTMP_PUSH_H
