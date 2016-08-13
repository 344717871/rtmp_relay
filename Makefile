CC = gcc  
C++ = g++  
LINK = g++  
  
LIBS = -lz -lm -lpthread

#must add -fPIC option  
CCFLAGS = $(COMPILER_FLAGS) -c -g -fPIC  
C++FLAGS = $(COMPILER_FLAGS) -c -g -fPIC  
  
TARGET=rtmp_relay 
  
INCLUDES = -I. -I./librtmp  
  
C++FILES = rtmp_relay.cpp LibRtmpSession.cpp RtmpPull.cpp mydebug.cpp DataQueue.cpp RtmpPush.cpp FLVParser.cpp
CFILES = ./librtmp/amf.c ./librtmp/hashswf.c ./librtmp/log.c ./librtmp/rtmp.c ./librtmp/parseurl.c
  
OBJFILE = $(CFILES:.c=.o) $(C++FILES:.cpp=.o)  
  
all:$(TARGET)  
  
$(TARGET): $(OBJFILE)  
	$(LINK) $^ $(LIBS) /usr/local/lib/libboost_system.a -Wall -fPIC -o $@
  
%.o:%.c  
	$(CC) -o $@ $(CCFLAGS) $< $(INCLUDES)  
  
%.o:%.cpp  
	$(C++) -o $@ $(C++FLAGS) $< $(INCLUDES)  
  
  
clean:  
	rm -rf $(TARGET)  
	rm -rf $(OBJFILE)  


