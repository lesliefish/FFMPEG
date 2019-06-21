#pragma once

#include <stdlib.h>
#include <string>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "sdl/SDL.h"
};


class FFmpegAudioPlayer
{
public:
    FFmpegAudioPlayer();

public:
    void play(const std::string& filePath);

private:
    // 初始化基本数据
    bool initDatas();

    // 获取流 音频流 视频流等
    bool getStreams();

    // 打开解码器
    bool avcodecOpen();
private:
    AVFormatContext* m_formatContext{ nullptr };
    AVCodecContext* m_codecContext{ nullptr };
    AVCodec* m_avCodec{ nullptr };
    // 解析器
    AVCodecParserContext* m_codecParserContext{ nullptr };

    std::string m_filePath;

    int m_audioStream{ -1 };
};