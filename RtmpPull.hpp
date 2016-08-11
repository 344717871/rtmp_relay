#ifndef RtmpPull_H
#define RtmpPull_H
#include <pthread.h>

class LibRtmpSession;

class RtmpPull
{
public:
	RtmpPull(char* szRtmpUrl);
	~RtmpPull();

	int Start();
	void Stop();
	void OnWork();
private:
	RtmpPull();

	int waitForConnect();
private:
	char _szRtmpUrl[512];
	
	LibRtmpSession* _rtmpSession;
	unsigned char* _pReadBuffer;

	int _iStartFlag;
	pthread_t threadId;

	int _iThreadEndFlag;
};

#endif//RtmpPull_H
