#include "SimpleSDLPlayer.h"
#include <future>
#include <iostream>

using namespace std;

//Refresh Event
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)

/*
 * @func   SimpleSDLPlayer::SimpleSDLPlayer 
 * @brief  构造函数
 * @param  [in]  int playerWidth  播放器宽度
 * @param  [in]  int playerHeight  播放器高度
 * @param  [in]  int yuvImagePixelWidth  yuv文件图像像素宽度
 * @param  [in]  int yuvImagePixelHeight  yuv文件图像像素高度
 * @param  [in]  const std::string & playerName  播放器名称
 * @return   
 */ 
SimpleSDLPlayer::SimpleSDLPlayer(int playerWidth, int playerHeight, int yuvImagePixelWidth, int yuvImagePixelHeight, const std::string& playerName /*= ""*/)
{
    m_playerWindowWidth = playerWidth;
    m_playerWindowHeight = playerHeight;
    m_imagePixelWidth = yuvImagePixelWidth;
    m_imagePixelHeight = yuvImagePixelHeight;
    m_playerName = playerName;

    if (!initSDLInfo())
        return;
}

SimpleSDLPlayer::~SimpleSDLPlayer()
{
}

/*
 * @func   SimpleSDLPlayer::initSDLInfo 
 * @brief  初始化SDL
 * @return bool  
 */ 
bool SimpleSDLPlayer::initSDLInfo()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        cout << "Could not initialize SDL : " << SDL_GetError() << endl;
        return false;
    }

    // 播放窗口创建
    m_playerWindow = SDL_CreateWindow(m_playerName.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_playerWindowWidth, m_playerWindowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_playerWindow)
    {
        cout << "SDL: could not create window - exiting: " << SDL_GetError() << endl;
        return false;
    }

    // 渲染
    m_sdlRenderer = SDL_CreateRenderer(m_playerWindow, -1, 0);
    if (!m_sdlRenderer)
    {
        cout << "SDL: could not create render - exiting: " << SDL_GetError() << endl;
        return false;
    }

    // 纹理
    m_sdlTexture = SDL_CreateTexture(m_sdlRenderer, m_pixformat, SDL_TEXTUREACCESS_STREAMING, m_imagePixelWidth, m_imagePixelHeight);
    if (!m_sdlTexture)
    {
        cout << "SDL: could not create texture - exiting: " << SDL_GetError() << endl;
        return false;
    }

    return true;
}

/*
 * @func   SimpleSDLPlayer::play 
 * @brief  播放文件
 * @param  [in]  const std::string & filePath  文件路径
 * @return bool  
 */ 
bool SimpleSDLPlayer::play(const std::string& filePath)
{
    FILE *fp = NULL;
    fp = fopen(filePath.c_str(), "rb+");
    if (fp == NULL) 
    {
        cout << "cannot open this file" << endl;
        return false;
    }

    bool isOver{ false };
    // 使用std::async异步执行刷新任务 相当于开子线程执行任务
    auto fut = std::async([&]
    {
        while (!isOver)
        {
            SDL_Event event;
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
            SDL_Delay(40);
        }
        isOver = false;
        //Break
        SDL_Event event;
        event.type = BREAK_EVENT;
        SDL_PushEvent(&event);

        return 0;
    });

    SDL_Event event;
    SDL_Rect sdlRect;

    while (true) 
    {
        const int bpp = 12;
        shared_ptr<char*> pBuffer = make_shared<char*>(new char[m_imagePixelWidth * m_imagePixelHeight * bpp / 8]);
        // 等待有效事件
        SDL_WaitEvent(&event);
        if (event.type == REFRESH_EVENT) 
        {
            if (fread(*pBuffer.get(), 1, m_imagePixelWidth*m_imagePixelHeight*bpp / 8, fp) != m_imagePixelWidth*m_imagePixelHeight*bpp / 8)
            {
                // Loop
                fseek(fp, 0, SEEK_SET);
                fread(*pBuffer.get(), 1, m_imagePixelWidth*m_imagePixelHeight*bpp / 8, fp);
            }

            SDL_UpdateTexture(m_sdlTexture, NULL, *pBuffer.get(), m_imagePixelWidth);

            //FIX: If window is resize
            sdlRect.x = 0;
            sdlRect.y = 0;
            sdlRect.w = m_playerWindowWidth;
            sdlRect.h = m_playerWindowHeight;

            SDL_RenderClear(m_sdlRenderer);
            SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, NULL, &sdlRect);
            SDL_RenderPresent(m_sdlRenderer);
        }
        else if (event.type == SDL_WINDOWEVENT) 
        {
            //If Resize
            SDL_GetWindowSize(m_playerWindow, &m_playerWindowWidth, &m_playerWindowHeight);
        }
        else if (event.type == SDL_QUIT) 
        {
            isOver = true;
        }
        else if (event.type == BREAK_EVENT) 
        {
            break;
        }
    }
    SDL_Quit();

    return true;
}


/*
SDL1.2 ---> SDL2.0 函数变化
一些重命名或替换的东西的摘要
一个简短的备忘单，其中一些旧的功能和其他东西：
SDL_SetVideoMode()：改为使用SDL_CreateWindow()   如果你想做经典的2D渲染而不是OpenGL，还可以使用SDL_CreateRenderer()
SDL_ListModes()：使用SDL_GetDisplayMode()/ SDL_GetNumDisplayModes()代替
SDL_UpdateRect()/ SDL_Flip()：使用SDL_RenderPresent()代替
SDL_Surface / 2D渲染：曲面仍然存在，但建议不要使用SDL_Surfaces，而是尽可能使用带有2D加速渲染器的SDL_Texture（SDL_CreateRenderer()）
SDL_VideoInfo：使用SDL_GetRendererInfo()/ SDL_GetRenderDriverInfo()代替
SDL_GetCurrentVideoDisplay()：使用SDL_GetWindowDisplayIndex()代替
SDL_VIDEORESIZE事件：新等效项为SDL_WINDOWEVENT_RESIZED
*/