#ifndef FLVParser_H
#define FLVParser_H

enum FLV_PAYLOAD_TYPE{
    SPS_PPS_TYPE,
    ASC_TYPE,
    VIDEO_I_TYPE,
    VIDEO_P_TYPE,
    AUDIO_TYPE,
    ONMETADATA_TYPE
};

typedef struct{
    enum FLV_PAYLOAD_TYPE _type;
    unsigned char* _data;
    int            _iLen;
    unsigned int   _uiTimestamp;
    unsigned char* _spsData;
    unsigned char* _ppsData;
    unsigned char* _ascData;
    int _iSpsLen;
    int _iPpsLen;
    int _iAscLen;
}FLVPlayInfo;

class FLVParser
{
public:
    FLVParser();
    ~FLVParser();

    FLVPlayInfo* parse(unsigned char* pFlvData, int iFlvLength, int& iLeftLength);

private:
    int readFLVHeader(unsigned char* pFlvData, int iFlvLength);
    FLVPlayInfo* readAudio(unsigned char* pFlvData, int iFlvLength, int& iLeftLength);
    FLVPlayInfo* readVideo(unsigned char* pFlvData, int iFlvLength, int& iLeftLength);
private:
    bool _bIsReadHeader;
};

#endif//FLVParser_H