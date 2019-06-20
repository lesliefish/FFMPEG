/** @file   SdlImageViewer.h
 *  @brief  SDL 图片预览
 *  @note
 *  @author lesliefish
 *  @date   2019/06/20
 */
#pragma once

#include "SDL/SDL.h"
#include <string>

namespace sdl2
{
    class SdlImageViewer
    {
    public:
        SdlImageViewer();
        ~SdlImageViewer();
    public:
        bool displayImage(const std::string& filePath);

    private:
        bool initSDLInfo();

    private:
        // 要渲染的窗口
        SDL_Window* m_window{ nullptr };

        // 窗口的表面
        SDL_Surface* m_windowSurface{ nullptr };

        // 我们将加载并显示在屏幕上的图像
        SDL_Surface* m_imageSurface{ nullptr };
    };
}