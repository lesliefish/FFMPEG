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
    // ��ʼ����������
    bool initDatas();

    // ��ȡ�� ��Ƶ�� ��Ƶ����
    bool getStreams();

    // �򿪽�����
    bool avcodecOpen();
private:
    AVFormatContext* m_formatContext{ nullptr };
    AVCodecContext* m_codecContext{ nullptr };
    AVCodec* m_avCodec{ nullptr };
    // ������
    AVCodecParserContext* m_codecParserContext{ nullptr };

    std::string m_filePath;

    int m_audioStream{ -1 };
};