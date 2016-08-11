#ifndef RTMP_PUSH_H
#define RTMP_PUSH_H
#include "DataQueue.hpp"
#include "LibRtmpSession.hpp"

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

    void DataHandle(DATA_QUEUE_ITEM* pItem);
    void MediaHandle(unsigned char* pMediaData, int iMediaLength);
    void AudioHandle(unsigned char* pAudioData, int iMediaLength, unsigned int uiTimestamp);
    void VideoHandle(unsigned char* pVideoData, int iMediaLength, unsigned int uiTimestamp);
private:
    char _szRtmpUrl[512];
    unsigned char _pAscData[2];
    unsigned char _pPpsSpsData[512];
    
    LibRtmpSession* _rtmpSession;
    unsigned char* _pReadBuffer;

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
