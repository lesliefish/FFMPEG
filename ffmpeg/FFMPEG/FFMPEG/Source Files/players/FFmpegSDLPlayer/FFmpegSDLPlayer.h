/** @file   FFmpegSDLPlayer.h
 *  @brief  FFmpegSDLPlayer 基于FFmpeg + SDL2.0播放
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
    // 初始化基本数据
    bool initDatas();
    // 初始化sdl2
    bool initSdlInfo();
    // 获取流 音频流 视频流等
    bool getStreams();
    // 打开解码器
    bool avcodecOpen();
private:
    // 文件
    std::string m_filePath{};
    const AVCodec* m_avCodec{ nullptr };

    // 包含码流参数较多的结构体
    AVFormatContext* m_formatCtx{ nullptr }; 
    
    // 解析器
    AVCodecParserContext* m_codecParserContext{ nullptr };
    
    // 变量结构体 包括编解码器类型、采样率、声道数等等
    AVCodecContext* m_codecContext{ nullptr }; 
    AVFrame* m_yuvFrame{ nullptr };
    AVPacket* m_avPacket{ nullptr };
    SwsContext* m_swsContext{ nullptr };

    SDL_Window *m_playerWindow{ nullptr };
    SDL_Renderer* m_sdlRenderer{ nullptr };
    SDL_Texture* m_sdlTexture{ nullptr };

    int m_audioStream{ -1 }; // 音频流id
    int m_videoStream{ -1 }; // 视频流id
};