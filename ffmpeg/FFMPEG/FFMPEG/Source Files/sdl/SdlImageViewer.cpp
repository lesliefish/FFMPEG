#include "SdlImageViewer.h"
#include <iostream>

using namespace std;

namespace sdl2
{
    namespace
    {
        const int screenWidth = 640;
        const int screenHeight = 480;
    }

    SdlImageViewer::SdlImageViewer()
    {
        if (!initSDLInfo())
        {
            exit(-1);
        }
    }

    SdlImageViewer::~SdlImageViewer()
    {
        SDL_FreeSurface(m_imageSurface);
        m_imageSurface = nullptr;

        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        SDL_Quit();
    }

    /*
     * @func   sdl2::SdlImageViewer::displayImage 
     * @brief  显示图像
     * @param  [in]  const std::string & filePath  
     * @return bool  
     */ 
    bool SdlImageViewer::displayImage(const std::string& filePath)
    {
        m_imageSurface = SDL_LoadBMP(filePath.c_str());
        if (!m_imageSurface)
        {
            cout << "SDL: could not load bmp - exiting: " << SDL_GetError() << endl;
            return false;
        }

        SDL_BlitSurface(m_imageSurface, nullptr, m_windowSurface, nullptr);

        SDL_UpdateWindowSurface(m_window);
        
        SDL_Event event;
        while (true)
        {
            SDL_WaitEvent(&event);
            if (event.type == SDL_QUIT)
            {
                break;
            }
            else if (event.type == SDL_WINDOWEVENT)
            {
                SDL_UpdateWindowSurface(m_window);
            }
        }

        return true;
    }

    bool SdlImageViewer::initSDLInfo()
    {
        if (SDL_Init(SDL_INIT_VIDEO))
        {
            cout << "Could not initialize SDL : " << SDL_GetError() << endl;
            return false;
        }

        // 窗口创建
        m_window = SDL_CreateWindow("Image Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
        if (!m_window)
        {
            cout << "SDL: could not create window - exiting: " << SDL_GetError() << endl;
            return false;
        }

        m_windowSurface = SDL_GetWindowSurface(m_window);

        return true;
    }

}

