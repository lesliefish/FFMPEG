/** @file   PlayVideoUseSDL.h
 *
 *  @brief  使用SDL2.0播放视频
 *  @note   updates from https://github.com/chelyaev/ffmpeg-tutorial
 *  @author lesliefish
 *  @date   2019/07/01
 */
#pragma 

#ifdef _WIN32
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
#include "sdl/sdl.h"
}
#endif

#include <stdio.h>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

int main(int argc, char *argv[])
{
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream;
    AVCodecContext  *pCodecCtxOrig = NULL;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVFrame         *pFrameYUV = NULL;
    AVPacket        packet;
    struct SwsContext *sws_ctx = NULL;

    SDL_Window *playerWindow{ nullptr };
    SDL_Renderer* sdlRenderer{ nullptr };
    SDL_Texture* sdlTexture{ nullptr };
    SDL_Rect        rect;
    SDL_Event       event;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

    const char* pFilePath = "test.mp4";
    // Open video file
    if (avformat_open_input(&pFormatCtx, pFilePath, NULL, NULL) != 0)
        return -1; // Couldn't open file

                   // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1; // Couldn't find stream information

                   // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, pFilePath, 0);

    // Find the first video stream
    videoStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream = i;
            break;
        }
    if (videoStream == -1)
        return -1; // Didn't find a video stream

                   // Get a pointer to the codec context for the video stream
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    // Copy context
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if (avcodec_copy_context(pCodecCtx, pCodecCtxOrig) != 0)
    {
        fprintf(stderr, "Couldn't copy codec context");
        return -1; // Error copying codec context
    }

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1; // Could not open codec

    // 申请视频帧数据
    pFrame = av_frame_alloc();
    // 复制帧数据
    pFrameYUV = av_frame_alloc();

    // Make a screen to put our video
    playerWindow = SDL_CreateWindow("SDL 2.0", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!playerWindow)
    {
        fprintf(stderr, "SDL: create window falied.\n");
        exit(1);
    }

    // 渲染
    sdlRenderer = SDL_CreateRenderer(playerWindow, -1, 0);
    if (!sdlRenderer)
    {
        fprintf(stderr, "SDL: could not create render - exiting: \n");
        return false;
    }

    // 纹理
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
    if (!sdlTexture)
    {
        fprintf(stderr, "SDL: could not create texture - exiting: \n");
        return false;
    }

    // initialize SWS context for software scaling
    sws_ctx = sws_getContext(pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_YUV420P,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    // Read frames and save first five frames to disk
    i = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0)
    {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream)
        {
            int ret = avcodec_send_packet(pCodecCtx, &packet);
            if (ret < 0)
            {
                fprintf(stderr, "Error sending a packet for decoding\n");
                continue;
            }
            ret = avcodec_receive_frame(pCodecCtx, pFrame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                continue;
            }
            else if (ret < 0)
            {
                fprintf(stderr, "Error during decoding\n");
                continue;
            }

            // SDL绘制处理 
            sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                nullptr, nullptr, nullptr);
            // 将帧数据转为播放窗口需要的大小
            sws_scale(sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
            sws_freeContext(sws_ctx);

            rect.x = 0;
            rect.y = 0;
            rect.w = pCodecCtx->width;
            rect.h = pCodecCtx->height;
            SDL_UpdateTexture(sdlTexture, &rect, pFrameYUV->data[0], pFrameYUV->linesize[0]);
            SDL_RenderClear(sdlRenderer);
            SDL_RenderCopy(sdlRenderer, sdlTexture, &rect, &rect);
            SDL_RenderPresent(sdlRenderer);
        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
        SDL_PollEvent(&event);
        switch (event.type) {
        case SDL_QUIT:
            SDL_Quit();
            exit(0);
            break;
        default:
            break;
        }

    }

    // Free the YUV frame
    av_frame_free(&pFrame);

    // Close the codec
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}