#include "SimpleSDLPlayer.h"
#include "module/VideoDecoder.h"
#include <future>
#include <QEventLoop>
#include <QTimer>

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

    //����  
    SDL_Window *sdlScreen;
    //��Ⱦ��  
    SDL_Renderer* sdlRenderer;
    //����  
    SDL_Texture* sdlTexture;
    //���νṹ  
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
