#include "FLVParser.hpp"
#include "mydebug.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#ifndef NULL
#define NULL 0
#endif

#define PREVIOUS_TAG_SIZE 4
#define FLV_ITEM_HEADER_SIZE 11

FLVParser::FLVParser():_bIsReadHeader(false)
{
}

FLVParser::~FLVParser()
{
}

FLVPlayInfo* FLVParser::parse(unsigned char * pFlvData, int iFlvLength, int& iLeftLength)
{
    int iOffset = 0;
    unsigned char* pCurrnetData = NULL;
    int iCurrent = iFlvLength;
    bool bReadHeaderFlag = false;
    
    if(!_bIsReadHeader)
    {
        iOffset = readFLVHeader(pFlvData, iFlvLength);
        if(iOffset <= 0)
        {
            return NULL;
        }
        _bIsReadHeader = true;
        bReadHeaderFlag = true;
        iCurrent -= iOffset;
    }
        iOffset, iFlvLength);
    if(iOffset >= iFlvLength)
    {
        return NULL;
    }

    pCurrnetData = pFlvData + iOffset;

    if(bReadHeaderFlag)
    {
        pCurrnetData  = pCurrnetData + PREVIOUS_TAG_SIZE;
        iCurrent -= PREVIOUS_TAG_SIZE;
    }
    
    pCurrnetData[0], pCurrnetData[1], pCurrnetData[2], pCurrnetData[3]);
    if(pCurrnetData[0] == 0x08)//audio
    {
        FLVPlayInfo* pAudioInfo = readAudio(pCurrnetData, iCurrent, iLeftLength);
        
        return pAudioInfo;
    }
    else if(pCurrnetData[0] == 0x09)//video
    {
        FLVPlayInfo* pVideoInfo = readVideo(pCurrnetData, iCurrent, iLeftLength);

        return pVideoInfo;
    }

    return NULL;//medatedata    
}

int FLVParser::readFLVHeader(unsigned char * pFlvData, int iFlvLength)
{
    if((pFlvData[0] == 0x46) && (pFlvData[1] == 0x4c) && (pFlvData[2] == 0x56))//FLVͷ
    {
        int iOffset = ((int)pFlvData[5]<<24) | ((int)pFlvData[6]<<16) | ((int)pFlvData[7]<<8) | ((int)pFlvData[8]);
        return iOffset;
    }
    else
    {
        return 0;
    }
}

FLVPlayInfo* FLVParser::readAudio(unsigned char * pFlvData, int iFlvLength, int& iLeftLength)
{
    FLVPlayInfo* pPayloadInfo = NULL;
    unsigned char* pCurrent   = pFlvData;
    int iOffset = FLV_ITEM_HEADER_SIZE;
    int iAudioLen = 0;
    unsigned int uiTimestamp = 0;

    iAudioLen   = ((int)pCurrent[1]<<16) | ((int)pCurrent[2]<<8) | pCurrent[3];
    uiTimestamp = ((int)pCurrent[7]<<24) | ((int)pCurrent[4]<<16) | ((int)pCurrent[5]<<8) | pCurrent[6];

    pCurrent += iOffset;
    iLeftLength = iFlvLength - FLV_ITEM_HEADER_SIZE - iAudioLen - PREVIOUS_TAG_SIZE;

    if((pCurrent[0] == 0xaf) && (pCurrent[1] == 0x00))//ASC FLAG
    {
        pPayloadInfo = (FLVPlayInfo*)malloc(sizeof(FLVPlayInfo));
        pPayloadInfo->_type    = ASC_TYPE;
        pPayloadInfo->_ascData = pCurrent+2;
        pPayloadInfo->_iAscLen = iAudioLen - 2;//without 0xaf 01
    }
    else if((pCurrent[0] == 0xaf) && (pCurrent[1] == 0x01))//Auido
    {
        pPayloadInfo = (FLVPlayInfo*)malloc(sizeof(FLVPlayInfo));
        pPayloadInfo->_type = AUDIO_TYPE;
        pPayloadInfo->_data = pCurrent + 2;
        pPayloadInfo->_iLen = iAudioLen - 2;//without 0xaf 01
        pPayloadInfo->_uiTimestamp = uiTimestamp;
    }

    return pPayloadInfo;
}

FLVPlayInfo* FLVParser::readVideo(unsigned char * pFlvData, int iFlvLength, int & iLeftLength)
{
    FLVPlayInfo* pPayloadInfo = NULL;
    unsigned char* pCurrent   = pFlvData;
    int iOffset = FLV_ITEM_HEADER_SIZE;
    int iVideoLen = 0;
    unsigned int uiTimestamp = 0;

    iVideoLen   = ((int)pCurrent[1]<<16) | ((int)pCurrent[2]<<8) | pCurrent[3];
    uiTimestamp = ((int)pCurrent[7]<<24) | ((int)pCurrent[4]<<16) | ((int)pCurrent[5]<<8) | pCurrent[6];

    pCurrent += iOffset;
    iLeftLength = iFlvLength - FLV_ITEM_HEADER_SIZE - iVideoLen - PREVIOUS_TAG_SIZE;

    if((pCurrent[0] == 0x17) && (pCurrent[1] == 0x00))//sps pps
    {
        pPayloadInfo = (FLVPlayInfo*)malloc(sizeof(FLVPlayInfo));
        pPayloadInfo->_type = SPS_PPS_TYPE;

        int iSpsLen = pCurrent[11];
        iSpsLen = iSpsLen << 8;
        iSpsLen += pCurrent[12];
        DebugString("sps len=%d\r\n", iSpsLen);
        pPayloadInfo->_spsData = pCurrent + 13;
        pPayloadInfo->_iSpsLen = iSpsLen;

        int iPpsStartPos = 13 + iSpsLen + 1;
        int iPpsLen = pCurrent[iPpsStartPos];
        iPpsLen = iPpsLen << 8;
        iPpsLen += pCurrent[iPpsStartPos+1];
        DebugString("pps len=%d\r\n", iPpsLen);
        pPayloadInfo->_ppsData = pCurrent + iPpsStartPos + 2;
        pPayloadInfo->_iPpsLen = iPpsLen;
    }
    else if((pCurrent[0] == 0x17) && (pCurrent[1] == 0x01))//I-Frame
    {
        pPayloadInfo = (FLVPlayInfo*)malloc(sizeof(FLVPlayInfo));
        pPayloadInfo->_type = VIDEO_I_TYPE;

        pPayloadInfo->_iLen = ((int)pCurrent[5] << 24) | ((int)pCurrent[6] << 16) | ((int)pCurrent[7] << 8) | pCurrent[8];
        pPayloadInfo->_data = pCurrent + 9;
        pPayloadInfo->_uiTimestamp = uiTimestamp;
    }
    else if(pCurrent[0] == 0x27)//P-Frame
    {
        pPayloadInfo = (FLVPlayInfo*)malloc(sizeof(FLVPlayInfo));
        pPayloadInfo->_type = VIDEO_P_TYPE;

        pPayloadInfo->_iLen = ((int)pCurrent[5] << 24) | ((int)pCurrent[6] << 16) | ((int)pCurrent[7] << 8) | pCurrent[8];
        pPayloadInfo->_data = pCurrent + 9;
        pPayloadInfo->_uiTimestamp = uiTimestamp;
    }
}

