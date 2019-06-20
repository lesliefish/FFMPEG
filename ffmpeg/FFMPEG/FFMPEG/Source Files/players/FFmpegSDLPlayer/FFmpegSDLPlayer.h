/** @file   FFmpegSDLPlayer.h
 *  @brief  FFmpegSDLPlayer ����FFmpeg + SDL2.0����
 *  @note
 *  @author lesliefish
 *  @date   2019/06/20
 */
#pragma once

#include <string>
#include <stdio.h>

#ifdef _WIN32
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
}
#endif

#include "sdl/sdl.h"

class FFmpegSDLPlayer
{
public:
    FFmpegSDLPlayer();

public:
    void play(const std::string& filePath);
    
private:
    // ��ʼ����������
    bool initDatas();
    // ��ʼ��sdl2
    bool initSdlInfo();
    // ��ȡ�� ��Ƶ�� ��Ƶ����
    bool getStreams();
    // �򿪽�����
    bool avcodecOpen();
private:
    // �ļ�
    std::string m_filePath{};
    const AVCodec* m_avCodec{ nullptr };

    // �������������϶�Ľṹ��
    AVFormatContext* m_formatCtx{ nullptr }; 
    
    // ������
    AVCodecParserContext* m_codecParserContext{ nullptr };
    
    // �����ṹ�� ��������������͡������ʡ��������ȵ�
    AVCodecContext* m_codecContext{ nullptr }; 
    AVFrame* m_yuvFrame{ nullptr };
    AVPacket* m_avPacket{ nullptr };
    SwsContext* m_swsContext{ nullptr };

    SDL_Window *m_playerWindow{ nullptr };
    SDL_Renderer* m_sdlRenderer{ nullptr };
    SDL_Texture* m_sdlTexture{ nullptr };

    int m_audioStream{ -1 }; // ��Ƶ��id
    int m_videoStream{ -1 }; // ��Ƶ��id
};