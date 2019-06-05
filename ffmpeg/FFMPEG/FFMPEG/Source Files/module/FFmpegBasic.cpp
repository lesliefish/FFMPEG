#include "FFmpegBasic.h"
#include <stdio.h>
#include <iostream>

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
};
#endif

struct URLProtocol;

/*
 * @func   FFmpegBasic::urlProtocolInfo 
 * @brief  FFmpeg类库支持的协议
 * @param  [in]  const int type  0:input,1:output
 * @return string  
 */ 
string FFmpegBasic::urlProtocolInfo(const int type)
{
    if (type != 0 && type != 1)
    {
        return string();
    }

    char *info = (char *)malloc(40000);
    memset(info, 0, 40000);

    struct URLProtocol *pup = nullptr;
    struct URLProtocol **pTemp = &pup;

    avio_enum_protocols((void**)pTemp, type);
    while ((*pTemp) != NULL) {
        sprintf(info, "%s%s;", info, avio_enum_protocols((void **)pTemp, type));//分号;间隔开
    }
    string str(info);
    free(info); info = nullptr;

    std::cout << str << std::endl;

    return move(str);
}
