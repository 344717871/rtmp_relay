#include "mydebug.hpp"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

const int DEBUG_BUFFER_SIZE = 8192;
const int DEBUG_ENABLE = 0;

void DebugString(const char* szDbg, ...)
{
    //仅仅调试使用, 非线程安全
    if(!DEBUG_ENABLE)
    {
        return;
    }
    va_list args;
    char szDebugBuffer[DEBUG_BUFFER_SIZE];

    va_start(args, szDbg);
    vsprintf((char*)szDebugBuffer, szDbg, args);
    va_end(args);
    FILE* pFile = fopen("debug.log", "ab+");
    fprintf(pFile, "%s", szDebugBuffer);
    fclose(pFile);
}

void DebugBody(char* szDscr, unsigned char* pData, int iLen)
{
    //仅仅调试使用, 非线程安全
    if(!DEBUG_ENABLE)
    {
        return;
    }
    int i = 0;
    int iCurrentLen = 0;
    char s_debugbuff[DEBUG_BUFFER_SIZE];

    if(iLen > 0)
    {
        iCurrentLen += sprintf(s_debugbuff+iCurrentLen, "%s\r\n", szDscr);
    }
    for(i = 0; i<iLen; i++)
    {
        if(iCurrentLen > 4000)
        {
            break;
        }
        if((i != 0) && ((i % 8) == 0))
        {
            iCurrentLen += sprintf(s_debugbuff+iCurrentLen, "\r\n");
        }
        iCurrentLen += sprintf(s_debugbuff+iCurrentLen, "%02x ", pData[i]);
    }
    sprintf(s_debugbuff+iCurrentLen, "\r\n");
    //printf(s_debugbuff);
    
    FILE* pFile = fopen("debugbody.log", "ab+");
    fprintf(pFile, "%s", s_debugbuff);
    fclose(pFile);
}

