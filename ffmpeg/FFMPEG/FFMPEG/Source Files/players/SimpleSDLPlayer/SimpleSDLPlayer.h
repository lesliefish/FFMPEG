/** @file   SimpleSDLPlayer.h
 *  @brief  SDL2.0 yuv²¥·ÅÆ÷
 *  @note   
 *  @author lesliefish
 *  @date   2019/06/20
 */
#pragma once

extern "C"
{
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
    // ²¥·ÅÆ÷¿í¶È
    int m_playerWindowWidth{ 0 };
    // ²¥·ÅÆ÷¸ß¶È
    int m_playerWindowHeight{ 0 };
    // Í¼ÏñÏñËØ¿í¶È
    int m_imagePixelWidth{ 0 };
    // Í¼ÏñÏñËØ¸ß¶È
    int m_imagePixelHeight{ 0 };

    std::string m_playerName{};
    SDL_Window *m_playerWindow{ nullptr };
    SDL_Renderer* m_sdlRenderer{ nullptr };
    SDL_Texture* m_sdlTexture{ nullptr };

    //IYUV: Y + U + V  (3 planes)
    //YV12: Y + V + U  (3 planes)
    Uint32 m_pixformat = SDL_PIXELFORMAT_IYUV;
};