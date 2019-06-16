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
#include "libavdevice/avdevice.h"
};
#endif

/*
 * @func   FFmpegBasic::urlProtocolInfo
 * @brief  FFmpeg类库支持的协议
 * @return std::map<std::string, std::string>
 */
std::map<std::string, std::string> FFmpegBasic::urlProtocolInfo()
{
    map<string, string> protocolMap{};
    string info{};

    void *pup = nullptr;
    // input
    avio_enum_protocols(&pup, 0);
    while (pup != nullptr)
    {
        const char *t = avio_enum_protocols(&pup, 0);
        info += string(t == nullptr ? "" : t) + string(";");
    }
    protocolMap["input"] = info;

    // output
    info.clear();
    avio_enum_protocols(&pup, 1);
    while (pup != nullptr)
    {
        const char *t = avio_enum_protocols(&pup, 0);
        info += string(t == nullptr ? "" : t) + string(";");
    }
    protocolMap["output"] = info;

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
    string info{};
    void *opaque = nullptr;

    const AVInputFormat *ifTemp{ nullptr };
    const AVOutputFormat *ofTemp{ nullptr };

    //input
    while ((ifTemp = av_demuxer_iterate(&opaque)) != nullptr)
    {
        info += string(ifTemp->name) + ";";
    }
    avFormatMap["input"] = info;

    // output
    info.clear();
    opaque = nullptr;
    while ((ofTemp = av_muxer_iterate(&opaque)) != nullptr)
    {
        info += string(ofTemp->name) + ";";
    }
    avFormatMap["output"] = string(info);

    return move(avFormatMap);
}


/*
 * @func   FFmpegBasic::avCoderInfo 
 * @brief  FFmpeg类库支持的编解码器
 * @return std::map<std::string, std::vector<std::pair<std::string, std::string>>>  
 */ 
std::map<std::string, std::vector<std::pair<std::string, std::string>>> 
FFmpegBasic::avCoderInfo()
{
    std::map<std::string, std::vector<std::pair<std::string, std::string>>> codeMap{};
    string info;
    void *opaque = nullptr;
    const AVCodec *coder = nullptr;

    while ((coder = av_codec_iterate(&opaque)) != nullptr)
    {
        string name = coder->name;
        string codeType;
        string type;
        switch (coder->type)
        {
        case AVMEDIA_TYPE_VIDEO:
            codeType = "video";
            break;
        case AVMEDIA_TYPE_AUDIO:
            codeType = "audio";
            break;
        default:
            codeType = "other";
            break;
        }

        if (coder->decode != nullptr)
        {
            codeMap["decode"].emplace_back(std::pair<string, string>(codeType, name));
        }
        else
        {
            codeMap["encode"].emplace_back(std::pair<string, string>(codeType, name));
        }
    }

    return codeMap;
}

/*
 * @func   FFmpegBasic::avFilterInfo 
 * @brief  FFmpeg类库支持的滤镜
 * @return std::string  
 */ 
std::string FFmpegBasic::avFilterInfo()
{
    string info;
    void *opaque = NULL;
    const AVFilter *filter;

    while ((filter = av_filter_iterate(&opaque)) != NULL)
    {
        info += string(filter->name) + ";";
    }

    return info;
}

/*
 * @func   FFmpegBasic::configurationInfo 
 * @brief  FFmpeg类库的配置信息
 * @return std::string  
 */ 
std::string FFmpegBasic::configurationInfo()
{
    return string(avdevice_configuration());
}

std::string FFmpegBasic::version()
{
    unsigned int v = avcodec_version();
    return to_string(v);
}
