/** @file   SdlImageViewer.h
 *  @brief  SDL ͼƬԤ��
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
        // Ҫ��Ⱦ�Ĵ���
        SDL_Window* m_window{ nullptr };

        // ���ڵı���
        SDL_Surface* m_windowSurface{ nullptr };

        // ���ǽ����ز���ʾ����Ļ�ϵ�ͼ��
        SDL_Surface* m_imageSurface{ nullptr };
    };
}