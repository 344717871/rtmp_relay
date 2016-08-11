#include "RtmpPull.hpp"
#include "LibRtmpSession.hpp"
#include "mydebug.hpp"
#include "DataQueue.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef NULL
#define NULL 0
#endif

#define READSIZE  (64*1024)
#define RTMP_PULL_SLEEP_INTERVAL (1*1000)

extern DataQueue g_DataQueue;

void* PullThreadCallback(void* pParam)
{
    RtmpPull* pThis = (RtmpPull*)pParam;
    if(pThis != NULL)
    {
        pThis->OnWork();
    }
}
RtmpPull::RtmpPull(char * szRtmpUrl):_rtmpSession(NULL)
    ,_pReadBuffer(NULL)
    ,_iStartFlag(0)
    ,_iThreadEndFlag(0)
{
    strcpy(_szRtmpUrl, szRtmpUrl);
    _rtmpSession = new LibRtmpSession(szRtmpUrl);

    _pReadBuffer = (unsigned char*)malloc(READSIZE);

    printf("RtmpPull construct....%s\r\n", szRtmpUrl);
}

RtmpPull::~RtmpPull()
{
    Stop();

    free(_pReadBuffer);
    _pReadBuffer = NULL;
    printf("RtmpPull destruct...\r\n");
}

int RtmpPull::Start()
{
    int iRet = 0;

    _iStartFlag = 1;
    _iThreadEndFlag = 0;

    printf("RtmpPull Start...\r\n");
    iRet = pthread_create(&threadId, NULL, PullThreadCallback, this);
    
    return iRet;
}

void RtmpPull::Stop()
{
    if(_iStartFlag == 0)
    {
        return;
    }
    _iStartFlag = 0;
    int iCount = 0;

    printf("RtmpPull Stop...\r\n");
    while(iCount < 200)
    {
        if(_iThreadEndFlag)
        {
            break;
        }
        usleep(RTMP_PULL_SLEEP_INTERVAL);
    }
    printf("RtmpPull Stop...finish\r\n");
}

int RtmpPull::waitForConnect()
{
    int iCount = 0;

    while(iCount < 5000)
    {
        if(!_iStartFlag)
        {
            return 0;
        }
        iCount++;
        usleep(RTMP_PULL_SLEEP_INTERVAL);
    }
    return 1;
}
void RtmpPull::OnWork()
{
    int iConnect = -1;
    int iStatus = -1;

    while(_iStartFlag)
    {
        if(iConnect != 0)
        {
            printf("RtmpPull is Connecting...%s\r\n", _szRtmpUrl);
            iConnect = _rtmpSession->Connect(RTMP_TYPE_PLAY);
            printf("RtmpPull Connecting...%s\r\n", (iConnect==0)?"ok":"error");
            if(iConnect != 0)
            {
                if(waitForConnect() == 0)
                {
                    break;
                }
                continue;
            }
        }

        int iRet = _rtmpSession->ReadData(_pReadBuffer, READSIZE);
        if(iRet > 0)
        {
            FILE* pFile = fopen("record.flv", "ab+");
            fwrite(_pReadBuffer, iRet, sizeof(unsigned char), pFile);
            fclose(pFile);

            g_DataQueue.InsertQueue(_pReadBuffer,iRet);
        }
        iStatus = _rtmpSession->GetReadStatus();
        if((iRet < 0) || (iStatus < 0))
        {
            iConnect = -1;
            _rtmpSession->DisConnect();
            if(waitForConnect() == 0)
            {
                break;
            }
            continue;
        }
        usleep(RTMP_PULL_SLEEP_INTERVAL);
    }
    _rtmpSession->DisConnect();
    _rtmpSession = NULL;
    _iThreadEndFlag = 1;
}

