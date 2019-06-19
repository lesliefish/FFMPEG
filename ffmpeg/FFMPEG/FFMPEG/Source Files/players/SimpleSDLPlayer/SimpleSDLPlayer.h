#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
}

#include <stdio.h>
#include <string>

class SimpleSDLPlayer
{
public:
    SimpleSDLPlayer(int playerWidth, int playerHeight, int yuvImagePixelWidth, int yuvImagePixelHeight, const std::string& playerName = "");
    ~SimpleSDLPlayer();

public:
    bool play(const std::string& filePath);

private:
    bool initSDLInfo();

private:
    // 播放器宽度
    int m_playerWindowWidth{ 0 };
    // 播放器高度
    int m_playerWindowHeight{ 0 };
    // 图像像素宽度
    int m_imagePixelWidth{ 0 };
    // 图像像素高度
    int m_imagePixelHeight{ 0 };

    std::string m_playerName{};
    SDL_Window *m_playerWindow{ nullptr };
    SDL_Renderer* m_sdlRenderer{ nullptr };
    SDL_Texture* m_sdlTexture{ nullptr };

    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    Uint32 m_pixformat = SDL_PIXELFORMAT_IYUV;
};