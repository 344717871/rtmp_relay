#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <memory.h>
#include "./librtmp/rtmp.h"
#include <stdlib.h>
#include <string.h>

#include "LibRtmpSession.hpp"
#include "RtmpPull.hpp"
#include "RtmpPush.hpp"
#include "DataQueue.hpp"

DataQueue g_DataQueue;

int main(int argn, char** argv)
{
    if(argn < 2)
    {
        printf("input parameter number is invalid...\r\n");
        return -1;
    }
    char* szRtmpPlayUrl = argv[1];
    char* szRtmpPushUrl = argv[2];
    
    RtmpPull* pRtmpPull = new RtmpPull(szRtmpPlayUrl);
    RtmpPush* pRtmpPush = new RtmpPush(szRtmpPushUrl);
    
    printf("rtmp_relay start play url=%s\r\n", szRtmpPlayUrl);
    printf("rtmp_relay start push url=%s\r\n", szRtmpPushUrl);

    pRtmpPush->Start();
    pRtmpPull->Start();
    char inputC = 0;
    do
    {
        inputC = getchar();
    }while(inputC != 'C');
    
    pRtmpPull->Stop();
    pRtmpPush->Stop();
    
    delete pRtmpPull;
    delete pRtmpPush;
    
    printf("rtmp relay end....\r\n");
}
#if 0
int main(int argn, char** argv)
{
    const int READSIZE = 64*1024;
    
    if(argn < 1)
    {
        printf("input parameter number is invalid...\r\n");
        return -1;
    }
    char* szRtmpPlayUrl = argv[1];

    printf("rtmp_relay start....\r\n");
    printf("rtmp_relay play url=%s\r\n", szRtmpPlayUrl);
    LibRtmpSession* pRtmpPlaySession = new LibRtmpSession(szRtmpPlayUrl);

    int iRet = pRtmpPlaySession->Connect(0);
    if(iRet != 0)
    {
        printf("rtmp play connect error....\r\n");
        return iRet;
    }
    printf("rtmp play connect ok....\r\n");
    
    unsigned char* pBuffer = (unsigned char*)malloc(READSIZE);

    int iStatus = 0;
    do{
        iRet = pRtmpPlaySession->ReadData(pBuffer, READSIZE);
        if(iRet > 0)
        {
            FILE* pFile = fopen("input.flv", "ab+");
            fwrite(pBuffer, iRet, sizeof(unsigned char), pFile);
            printf("Rtmp read %d....\r\n", iRet);
            fclose(pFile);
        }
        else
        {
            printf("rtmp ReadData return %d.\r\n", iRet);
        }
        iStatus = pRtmpPlaySession->GetReadStatus();
    }while((iRet >= 0) && (iStatus >= 0));

    printf("ReadData end...iRet=%d, iStatus=%d\r\n", iRet, iStatus);
    pRtmpPlaySession->DisConnect();
    printf("rtmp play DisConnect....\r\n");
    
    delete pRtmpPlaySession;
}
#endif


