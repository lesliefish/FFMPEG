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
 * @brief  FFmpeg���֧�ֵ�Э��
 * @return std::map<std::string, std::string>  
 */ 
std::map<std::string, std::string> FFmpegBasic::urlProtocolInfo()
{
    map<string, string> protocolMap{};
    char *info = (char *)malloc(40000);
    memset(info, 0, 40000);

    struct URLProtocol *pup = nullptr;
    struct URLProtocol **pTemp = &pup;

    // input
    avio_enum_protocols((void**)pTemp, 0);
    while ((*pTemp) != NULL) {
        sprintf(info, "%s%s;", info, avio_enum_protocols((void **)pTemp, 0));//�ֺ�;�����
    }
    protocolMap["input"] = string(info);

    // output
    memset(info, 0, 40000);
    avio_enum_protocols((void**)pTemp, 1);
    while ((*pTemp) != NULL) {
        sprintf(info, "%s%s;", info, avio_enum_protocols((void **)pTemp, 1));//�ֺ�;�����
    }

    protocolMap["output"] = string(info);

    // �ڴ��ͷ�
    free(info); info = nullptr;

    return move(protocolMap);
}
