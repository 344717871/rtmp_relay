#include "RtmpPush.hpp"
#include "sps_decode.h"
#include "librtmp/rtmp.h"
#include "mydebug.hpp"

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
    ,_pReadBuffer(NULL)
    ,_iStartFlag(0)
    ,_iThreadEndFlag(0)
    ,_usASCFlag(0)
    ,_iWidth(0)
    ,_iHeigth(0)
    ,_iFps(0)
    ,_iConnect(-1)
{
    strcpy(_szRtmpUrl, szRtmpUrl);
    _rtmpSession = new LibRtmpSession(szRtmpUrl);

    _pReadBuffer = (unsigned char*)malloc(READSIZE);

    printf("RtmpPush construct....%s\r\n", szRtmpUrl);
}

RtmpPush::~RtmpPush()
{
    Stop();

    free(_pReadBuffer);
    _pReadBuffer = NULL;
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
        
        DataHandle(pItem);
        usleep(RTMP_PUSH_SLEEP_INTERVAL);
    }
    _rtmpSession->DisConnect();
    _rtmpSession = NULL;
    _iThreadEndFlag = 1;
}

void RtmpPush::DataHandle(DATA_QUEUE_ITEM* pItem)
{
    if(pItem == NULL)
    {
        return;
    }
    unsigned char* pData = pItem->_pData;
    int iLength = pItem->_iLength;

    if((pData[0] == 0x46) && (pData[1] == 0x4c) && (pData[2] == 0x56))//FLVͷ
    {
        unsigned int uiOffset = ((int)pData[5]<<24) | ((int)pData[6]<<16) | ((int)pData[7]<<8) | ((int)pData[8]);
        unsigned char* pMediaData = pData + uiOffset + 4;
        int iMediaLength = iLength - uiOffset - 4;

        MediaHandle(pMediaData, iMediaLength);
    }
    else if((pData[0] == 0x08) || (pData[0] == 0x09))
    {
        unsigned char* pMediaData = pData;
        int iMediaLength = iLength;
        MediaHandle(pMediaData, iMediaLength);
    }
}

void RtmpPush::MediaHandle(unsigned char * pMediaData,int iMediaLength)
{
    if(pMediaData == NULL)
    {
        return;
    }
    if(iMediaLength <= 0)
    {
        return;
    }
    unsigned char ucMediaType = pMediaData[0];
    if(ucMediaType == 0x08)//audio
    {
        int iAudioLen = ((int)pMediaData[1] << 16) | ((int)pMediaData[2] << 8) | (int)pMediaData[3];
        unsigned int uiTimestamp = ((int)pMediaData[7] << 24) | ((int)pMediaData[4] << 16) | (int)(pMediaData[5] << 8) | pMediaData[6] ;
        int iStreamID = ((int)pMediaData[8] << 16) | ((int)pMediaData[9] << 8) | (int)pMediaData[10];
        unsigned char* pAudioData = pMediaData + RTMP_MESSAGE_HEADER_SIZE;

        AudioHandle(pAudioData, iAudioLen, uiTimestamp);
        if((pAudioData + iAudioLen + RTMP_PRE_FRAME_OFFET) < (pMediaData + iMediaLength))
        {
            unsigned char* pNewMediaData = (pAudioData + iAudioLen + RTMP_PRE_FRAME_OFFET);
            int iNewLen = iMediaLength - iAudioLen - RTMP_MESSAGE_HEADER_SIZE - RTMP_PRE_FRAME_OFFET;
            MediaHandle(pNewMediaData, iNewLen);
        }
    }
    else if(ucMediaType == 0x09)//video
    {
        int iVideoLen = ((int)pMediaData[1] << 16) | ((int)pMediaData[2] << 8) | (int)pMediaData[3];
        unsigned int uiTimestamp = ((int)pMediaData[7] << 24) | ((int)pMediaData[4] << 16) | (int)(pMediaData[5] << 8) | pMediaData[6] ;
        int iStreamID = ((int)pMediaData[8] << 16) | ((int)pMediaData[9] << 8) | pMediaData[10];
        unsigned char* pVideoData = pMediaData + RTMP_MESSAGE_HEADER_SIZE;
        VideoHandle(pVideoData, iVideoLen, uiTimestamp);
        if((pVideoData + iVideoLen + RTMP_PRE_FRAME_OFFET) < (pMediaData + iMediaLength))
        {
            unsigned char* pNewMediaData = (pVideoData + iVideoLen + RTMP_PRE_FRAME_OFFET);
            int iNewLen = iMediaLength - iVideoLen - RTMP_MESSAGE_HEADER_SIZE - RTMP_PRE_FRAME_OFFET;
            MediaHandle(pNewMediaData, iNewLen);
        }
    }
}

void RtmpPush::AudioHandle(unsigned char * pAudioData, int iAudioLength, unsigned int uiTimestamp)
{
    if(pAudioData[0] == 0xaf)
    {
        if((pAudioData[1] == 0x00) && (iAudioLength == 4))//ASC configure
        {
            //ASC FLAG: xxxx xaaa aooo o111, example:0x13 90, 0b0001 0011 1001 0000
            _usASCFlag = pAudioData[2];
            _usASCFlag = (_usASCFlag << 8) | pAudioData[3];
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
        else if(pAudioData[1] == 0x01)//AAC Audio Data
        {
            int iRet = _rtmpSession->SendAACData(pAudioData+2, iAudioLength-2, uiTimestamp);
            if(iRet < 0)
            {
                DebugString("SendAACData error return %d\r\n", iRet);
                _iConnect = -1;
                _rtmpSession->DisConnect();
            }
        }
        else
        {
            DebugString("###Unknown: %02x %02x\r\n", pAudioData[0], pAudioData[1]);
        }
    }
    else
    {
        DebugString("###Unknown: %02x %02x\r\n", pAudioData[0], pAudioData[1]);
    }
}

void RtmpPush::VideoHandle(unsigned char * pVideoData,int iVideoLength,unsigned int uiTimestamp)
{
    if(pVideoData[0] == 0x17)
    {
        if(pVideoData[1] == 0x01)//I-Frame
        {
            int iH264Length = 0;
            int iTemp = 0;
            unsigned char* pH264Data = NULL;
            
            iTemp = pVideoData[5];
            iH264Length += iTemp << 24;
            iTemp = pVideoData[6];
            iH264Length += iTemp << 16;
            iTemp = pVideoData[7];
            iH264Length += iTemp << 8;
            iH264Length += pVideoData[8];

            pH264Data = pVideoData + 9;

            int iRet = _rtmpSession->SendH264Packet(pH264Data, iH264Length, 1, uiTimestamp);
            if(iRet < 0)
            {
                DebugString("SendVideoData error return %d\r\n", iRet);
                _iConnect = -1;
                _rtmpSession->DisConnect();
            }
        }
        else if(pVideoData[1] == 0x00)//pps sps frame
        {
            unsigned char ucSps[80];
            unsigned char ucPps[80];
            int iSpsLen = 0;
            int iPpsLen = 0;
            int iPpsStartPos = 0;

            memset(ucSps, 0, sizeof(ucSps));
            memset(ucPps, 0, sizeof(ucPps));
            iSpsLen = pVideoData[11];
            iSpsLen = iSpsLen << 8;
            iSpsLen += pVideoData[12];
            DebugString("sps len=%d\r\n", iSpsLen);
            memcpy(ucSps, pVideoData+13, iSpsLen);

            iPpsStartPos = 13 + iSpsLen + 1;

            iPpsLen = pVideoData[iPpsStartPos];
            iPpsLen = iPpsLen << 8;
            iPpsLen += pVideoData[iPpsStartPos+1];
            DebugString("pps len=%d\r\n", iPpsLen);
            memcpy(ucPps, pVideoData + iPpsStartPos + 2, iPpsLen);

            int* Width = &_iWidth;
            int* Height = &_iHeigth;
            int* Fps = &_iFps;
            h264_decode_sps(ucSps, iSpsLen, &Width, &Height, &Fps);

            DebugString("RtmpPush sps info: %d x %d, %d\r\n", _iWidth, _iHeigth, _iFps);
            int iRet = _rtmpSession->SendVideoSpsPps(ucPps, iPpsLen, ucSps, iSpsLen);
            if(iRet < 0)
            {
                DebugString("SendVideoSpsPps error return %d\r\n", iRet);
                _rtmpSession->DisConnect();
            }
            unsigned char* pNewMediaData = pVideoData + iPpsStartPos + 2 + iPpsLen;
        }
    }
    else if(pVideoData[0] == 0x27)//P-Frame
    {
        int iH264Length = 0;
        int iTemp = 0;
        unsigned char* pH264Data = NULL;
            
        iTemp = pVideoData[5];
        iH264Length += iTemp << 24;
        iTemp = pVideoData[6];
        iH264Length += iTemp << 16;
        iTemp = pVideoData[7];
        iH264Length += iTemp << 8;
        iH264Length += pVideoData[8];

        pH264Data = pVideoData + 9;

        int iRet = _rtmpSession->SendH264Packet(pH264Data, iH264Length, 0, uiTimestamp);
        if(iRet < 0)
        {
            DebugString("SendVideoData error return %d\r\n", iRet);
            _rtmpSession->DisConnect();
            _iConnect = -1;
        }
    }
}

