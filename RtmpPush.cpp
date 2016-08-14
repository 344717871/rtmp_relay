#include "RtmpPush.hpp"
#include "sps_decode.h"
#include "librtmp/rtmp.h"
#include "mydebug.hpp"

#ifndef NULL
#define NULL 0
#endif

#define READSIZE  (64*1024)

#define RTMP_MESSAGE_HEADER_SIZE 11
#define RTMP_PRE_FRAME_OFFET     4

#define RTMP_PUSH_SLEEP_INTERVAL (5*1000)

extern DataQueue g_DataQueue;

void* PushThreadCallback(void* pParam)
{
    RtmpPush* pThis = (RtmpPush*)pParam;
    if(pThis != NULL)
    {
        pThis->OnWork();
    }
}

RtmpPush::RtmpPush(char* szRtmpUrl):_rtmpSession(NULL)
    ,_iStartFlag(0)
    ,_iThreadEndFlag(0)
    ,_usASCFlag(0)
    ,_iWidth(0)
    ,_iHeigth(0)
    ,_iFps(0)
    ,_iConnect(-1)
    ,_flvParser(NULL)
{
    strcpy(_szRtmpUrl, szRtmpUrl);
    _rtmpSession = new LibRtmpSession(szRtmpUrl);
    _flvParser = new FLVParser();

    printf("RtmpPush construct....%s\r\n", szRtmpUrl);
}

RtmpPush::~RtmpPush()
{
    Stop();

    delete _flvParser;
    printf("RtmpPush destruct...\r\n");
}

int RtmpPush::Start()
{
    int iRet = 0;

    _iStartFlag = 1;
    _iThreadEndFlag = 0;

    printf("RtmpPush Start...\r\n");
    iRet = pthread_create(&threadId, NULL, PushThreadCallback, this);
    
    return iRet;
}

void RtmpPush::Stop()
{
    if(_iStartFlag == 0)
    {
        return;
    }
    _iStartFlag = 0;
    int iCount = 0;

    printf("RtmpPush Stop...\r\n");
    while(iCount < 200)
    {
        if(_iThreadEndFlag)
        {
            break;
        }
        usleep(RTMP_PUSH_SLEEP_INTERVAL);
    }
    printf("RtmpPush Stop...finish\r\n");
}

int RtmpPush::waitForConnect()
{
    int iCount = 0;

    while(iCount < 1000)
    {
        if(!_iStartFlag)
        {
            return 0;
        }
        iCount++;
        usleep(RTMP_PUSH_SLEEP_INTERVAL);
    }
    return 1;
}
void RtmpPush::OnWork()
{
    int iStatus = -1;

    while(_iStartFlag)
    {
        if(_iConnect != 0)
        {
            printf("RtmpPush is Connecting %s...\r\n", _szRtmpUrl);
            _iConnect = _rtmpSession->Connect(RTMP_TYPE_PUSH);
            printf("RtmpPush Connecting...%s\r\n", (_iConnect==0)?"ok":"error");
            if(_iConnect != 0)
            {
                if(waitForConnect() == 0)
                {
                    break;
                }
                continue;
            }
        }
        DATA_QUEUE_ITEM* pItem = g_DataQueue.GetAndReleaseQueue();

        if(pItem == NULL)
        {
            usleep(RTMP_PUSH_SLEEP_INTERVAL);
            continue;
        }
        /*
        char szDebug[80];
        sprintf(szDebug, "RtmpPush get %d bytes from queue....", pItem->_iLength);
        int iDebugLen = (pItem->_iLength> 200) ? 200 : pItem->_iLength;
        DebugBody(szDebug, pItem->_pData, iDebugLen);
        */
        unsigned char* pData = pItem->_pData;
        int iLength = pItem->_iLength;

        DataHandle(pData, iLength);
        free(pData);
        free(pItem);
        usleep(RTMP_PUSH_SLEEP_INTERVAL);
    }
    _rtmpSession->DisConnect();
    _rtmpSession = NULL;
    _iThreadEndFlag = 1;
}

void RtmpPush::DataHandle(unsigned char* pData, int iLength)
{
    if(pData == NULL)
    {
        return;
    }

    if(iLength <= 0)
    {
        return;
    }
    int iLeftLength = 0;

    FLVPlayInfo* pFlvInfo = _flvParser->parse(pData, iLength, iLeftLength);

    if(pFlvInfo != NULL)
    {
        if(pFlvInfo->_type == ASC_TYPE)
        {
            unsigned char* pAudioData = pFlvInfo->_ascData;
            //ASC FLAG: xxxx xaaa aooo o111, example:0x13 90, 0b0001 0011 1001 0000
            _usASCFlag = pAudioData[0];
            _usASCFlag = (_usASCFlag << 8) | pAudioData[1];
            _rtmpSession->GetASCInfo(_usASCFlag);
            int iRet = _rtmpSession->SendAudioSpecificConfig(_usASCFlag);
            if(iRet < 0)
            {
                DebugString("SendAudioSpecificConfig error return %d\r\n", iRet);
                _iConnect = -1;
                _rtmpSession->DisConnect();
            }
            DebugString("AudioHandle, ASC_flag=0x%04x, %d, %d, %d\r\n", 
                _usASCFlag, _rtmpSession->GetAACType(), _rtmpSession->GetSampleRate(),
                _rtmpSession->GetChannels());
        }
        else if(pFlvInfo->_type == AUDIO_TYPE)
        {
            unsigned char* pAudioData = pFlvInfo->_data;
            int iAudioLength = pFlvInfo->_iLen;
            unsigned int uiTimestamp = pFlvInfo->_uiTimestamp;

            int iRet = _rtmpSession->SendAACData(pAudioData, iAudioLength, uiTimestamp);
            if(iRet < 0)
            {
                DebugString("SendAACData error return %d\r\n", iRet);
                _iConnect = -1;
                _rtmpSession->DisConnect();
            }
        }
        else if(pFlvInfo->_type == SPS_PPS_TYPE)
        {
            int iRet = _rtmpSession->SendVideoSpsPps(pFlvInfo->_ppsData, 
                pFlvInfo->_iPpsLen, pFlvInfo->_spsData, pFlvInfo->_iSpsLen);
            if(iRet < 0)
            {
                DebugString("SendVideoSpsPps error return %d\r\n", iRet);
                _rtmpSession->DisConnect();
            }
        }
        else if((pFlvInfo->_type == VIDEO_I_TYPE) || (pFlvInfo->_type == VIDEO_P_TYPE))
        {
            unsigned char* pH264Data = pFlvInfo->_data;
            int iH264Length = pFlvInfo->_iLen;
            unsigned int uiTimestamp = pFlvInfo->_uiTimestamp;
    
            int iRet = _rtmpSession->SendH264Packet(pH264Data, iH264Length, (pFlvInfo->_type == VIDEO_I_TYPE), uiTimestamp);
            if(iRet < 0)
            {
                DebugString("SendVideoData error return %d\r\n", iRet);
                _iConnect = -1;
                _rtmpSession->DisConnect();
            }
        }
        free(pFlvInfo);
    }
    
    if(iLeftLength <= 0)
    {
        return;
    }
    else
    {
        pData = pData + iLength - iLeftLength;
        DataHandle(pData, iLeftLength);
    }
}

