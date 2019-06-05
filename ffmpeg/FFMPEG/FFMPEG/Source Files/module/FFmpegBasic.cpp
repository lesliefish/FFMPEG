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
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> FFmpegBasic::urlProtocolInfo()
{
    map<string, string> protocolMap{};
    char *info = new char[40000];
    memset(info, 0, 40000);

    struct URLProtocol *pup = nullptr;
    struct URLProtocol **pTemp = &pup;

    // input
    avio_enum_protocols((void**)pTemp, 0);
    while ((*pTemp) != NULL) 
    {
        sprintf(info, "%s%s;", info, avio_enum_protocols((void **)pTemp, 0));//分号;间隔开
    }
    protocolMap["input"] = string(info);

    // output
    memset(info, 0, 40000);
    avio_enum_protocols((void**)pTemp, 1);
    while ((*pTemp) != NULL) 
    {
        sprintf(info, "%s%s;", info, avio_enum_protocols((void **)pTemp, 1));//分号;间隔开
    }

    protocolMap["output"] = string(info);

    // 内存释放
    delete[]info; info = nullptr;

    return move(protocolMap);
}

/*
 * @func   FFmpegBasic::avFormatInfo
 * @brief  FFmpeg类库支持的封装格式
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> FFmpegBasic::avFormatInfo()
{
    std::map<std::string, std::string> avFormatMap{};
    char *info = new char[40000];
    memset(info, 0, 40000);

    AVInputFormat *ifTemp = av_iformat_next(NULL);
    AVOutputFormat *ofTemp = av_oformat_next(NULL);
    //Input
    while (ifTemp != NULL) 
    {
        sprintf(info, "%s%s;", info, ifTemp->name);
        ifTemp = ifTemp->next;
    }
    avFormatMap["input"] = string(info);

    //Output
    memset(info, 0, 40000);
    while (ofTemp != NULL) 
    {
        sprintf(info, "%s%s;", info, ofTemp->name);
        ofTemp = ofTemp->next;
    }
    avFormatMap["output"] = string(info);

    // 内存释放
    delete[]info; info = nullptr;

    return move(avFormatMap);
}
