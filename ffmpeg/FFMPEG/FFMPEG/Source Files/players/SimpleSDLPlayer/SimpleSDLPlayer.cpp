#include "SimpleSDLPlayer.h"
#include <future>
#include <iostream>

using namespace std;

//Refresh Event
#define REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BREAK_EVENT  (SDL_USEREVENT + 2)

/*
 * @func   SimpleSDLPlayer::SimpleSDLPlayer 
 * @brief  ���캯��
 * @param  [in]  int playerWidth  ���������
 * @param  [in]  int playerHeight  �������߶�
 * @param  [in]  int yuvImagePixelWidth  yuv�ļ�ͼ�����ؿ��
 * @param  [in]  int yuvImagePixelHeight  yuv�ļ�ͼ�����ظ߶�
 * @param  [in]  const std::string & playerName  ����������
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
 * @brief  ��ʼ��SDL
 * @return bool  
 */ 
bool SimpleSDLPlayer::initSDLInfo()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        cout << "Could not initialize SDL : " << SDL_GetError() << endl;
        return false;
    }

    // ���Ŵ��ڴ���
    m_playerWindow = SDL_CreateWindow(m_playerName.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_playerWindowWidth, m_playerWindowHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (!m_playerWindow)
    {
        cout << "SDL: could not create window - exiting: " << SDL_GetError() << endl;
        return false;
    }

    // ��Ⱦ
    m_sdlRenderer = SDL_CreateRenderer(m_playerWindow, -1, 0);
    if (!m_sdlRenderer)
    {
        cout << "SDL: could not create render - exiting: " << SDL_GetError() << endl;
        return false;
    }

    // ����
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
 * @brief  �����ļ�
 * @param  [in]  const std::string & filePath  �ļ�·��
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
    // ʹ��std::async�첽ִ��ˢ������ �൱�ڿ����߳�ִ������
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
        // �ȴ���Ч�¼�
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
SDL1.2 ---> SDL2.0 �����仯
һЩ���������滻�Ķ�����ժҪ
һ����̵ı�����������һЩ�ɵĹ��ܺ�����������
SDL_SetVideoMode()����Ϊʹ��SDL_CreateWindow()   ��������������2D��Ⱦ������OpenGL��������ʹ��SDL_CreateRenderer()
SDL_ListModes()��ʹ��SDL_GetDisplayMode()/ SDL_GetNumDisplayModes()����
SDL_UpdateRect()/ SDL_Flip()��ʹ��SDL_RenderPresent()����
SDL_Surface / 2D��Ⱦ��������Ȼ���ڣ������鲻Ҫʹ��SDL_Surfaces�����Ǿ�����ʹ�ô���2D������Ⱦ����SDL_Texture��SDL_CreateRenderer()��
SDL_VideoInfo��ʹ��SDL_GetRendererInfo()/ SDL_GetRenderDriverInfo()����
SDL_GetCurrentVideoDisplay()��ʹ��SDL_GetWindowDisplayIndex()����
SDL_VIDEORESIZE�¼����µ�Ч��ΪSDL_WINDOWEVENT_RESIZED
*/