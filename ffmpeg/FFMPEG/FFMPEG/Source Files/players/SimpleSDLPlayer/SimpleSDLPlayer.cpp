#include "SimpleSDLPlayer.h"
#include "module/VideoDecoder.h"
#include <future>
#include <QEventLoop>
#include <QTimer>

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

SimpleSDLPlayer::SimpleSDLPlayer(QObject *parent)
    : QObject(parent)
{


}

SimpleSDLPlayer::~SimpleSDLPlayer()
{
}

void SimpleSDLPlayer::play(const std::string& filePath)
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return;
    }

    //SDL
    int screenWidth = 640, screenHeight = 480;
    const int pixelWidth = 640, pixelHeight = 480;
    const int bpp = 12;
    unsigned char buffer[pixelWidth * pixelHeight * bpp / 8];

    //窗口  
    SDL_Window *sdlScreen;
    //渲染器  
    SDL_Renderer* sdlRenderer;
    //纹理  
    SDL_Texture* sdlTexture;
    //矩形结构  
    SDL_Rect sdlRect;

    sdlScreen = SDL_CreateWindow("Simple SDL Player",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screenWidth, screenHeight,
        SDL_WINDOW_SHOWN);

    sdlRenderer = SDL_CreateRenderer(sdlScreen, -1, SDL_RENDERER_ACCELERATED);

    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, pixelWidth, pixelHeight);
    if (sdlTexture == NULL)
    {
        printf("SDL: could not create SDL_Texture - exiting:%s\n", SDL_GetError());
        return;
    }

    
    VideoDecoder decoder;
    QEventLoop loop;
    auto fut = std::async(std::launch::deferred, [&]
    {
        decoder.doDecode("C:\\Users\\yulei10\\Videos\\aa.mp4", "C:\\Users\\yulei10\\Videos\\aa");
        loop.quit();
    });

    connect(&decoder, &VideoDecoder::signalDecodeEvent, [&](const AVFrame& frame) 
    {
        SDL_UpdateTexture(sdlTexture, NULL, frame.data[0], frame.linesize[0]);
        sdlRect.x = 0;
        sdlRect.y = 0;
        sdlRect.w = screenWidth;
        sdlRect.h = screenHeight;
        SDL_RenderClear(sdlRenderer);
        SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);
        SDL_RenderPresent(sdlRenderer);
    });

    QTimer::singleShot(1000, [&] 
    {
        fut.get();
    });
    loop.exec();
    SDL_DestroyTexture(sdlTexture);
    SDL_DestroyRenderer(sdlRenderer);
    SDL_DestroyWindow(sdlScreen);
    SDL_Quit();
}
