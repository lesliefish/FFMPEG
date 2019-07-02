/** @file   PlayVideoUseSDL.h
 *
 *  @brief  使用SDL2.0播放视频
 *  @note   updates from https://github.com/chelyaev/ffmpeg-tutorial
 *  @author lesliefish
 *  @date   2019/07/01
 */
#pragma 

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
}

#include "sdl/sdl.h"
#include <stdio.h>
#include <future>

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#define  REFRESH_EVENT SDL_USEREVENT + 1000

class PlayVideoUseSDl2
{
public:
    static int test()
    {
        AVFormatContext *pFormatCtx = NULL;
        int             i, videoStream;
        AVCodecContext  *pCodecCtx = NULL;
        AVCodec         *pCodec = NULL;
        AVFrame         *pFrame = NULL;
        AVFrame         *pFrameYUV = NULL;
        AVPacket        packet;
        struct SwsContext *sws_ctx = NULL;

        SDL_Window* playerWindow{ nullptr };
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
        // 打开视频文件
        if (avformat_open_input(&pFormatCtx, pFilePath, NULL, NULL) != 0)
            return -1; // 打开失败

        // 找流信息
        if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
            return -1; // 找不到流

        // 视频文件信息
        av_dump_format(pFormatCtx, 0, pFilePath, 0);

        // 找到视频流
        videoStream = -1;
        for (i = 0; i < pFormatCtx->nb_streams; i++)
        {
            if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
            {
                videoStream = i;
                break;
            }
        }

        if (videoStream == -1)
            return -1; // 未找到视频流

        // Get a pointer to the codec context for the video stream
        pCodecCtx = pFormatCtx->streams[videoStream]->codec;
        // 视频流的解码器
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (pCodec == NULL)
        {
            fprintf(stderr, "Unsupported codec!\n");
            return -1; // 解码器未找到
        }

        // 打开解码器
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
            return -1; // 失败

        // 申请视频帧数据
        pFrame = av_frame_alloc();
        // 复制帧数据
        pFrameYUV = av_frame_alloc();

        //播放窗口创建
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

        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
        shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
        av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, *buffer.get(), AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

        // 是否读数据完毕
        bool isReadOver{ false };

        // 线程处理部分 每隔40ms读取一帧视频数据
        auto fut = std::async([&]
        {
            SDL_Event event;
            while (!isReadOver)
            {
                event.type = REFRESH_EVENT;
                SDL_PushEvent(&event);
                SDL_Delay(40); // 40ms取一帧数据, 刷新一次
            }

            // 读完数据后发送关闭消息
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        });

        // 事件监听
        for (;;)
        {
            SDL_WaitEvent(&event);
            if (event.type == REFRESH_EVENT)
            {
                // 读帧数据并播放
                if (av_read_frame(pFormatCtx, &packet) >= 0)
                {
                    // 是否视频流packet?
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

                        // 初始化SWS 
                        sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                            pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                            nullptr, nullptr, nullptr);
                        // 将帧数据转为播放窗口需要的大小
                        sws_scale(sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                            pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                        // 释放av_read_frame申请的packet
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
                    else
                    {
                        // 每次获取的不是视频流 就会造成卡顿40ms 需要主动去读取下一帧视频
                        SDL_Event event;
                        event.type = REFRESH_EVENT;
                        SDL_PushEvent(&event);
                    }

                    av_free_packet(&packet);
                }
                else
                {
                    isReadOver = true;
                }
            }
            else if (event.type == SDL_QUIT)
            {
                SDL_Quit();
                exit(0);
            }
        }

        // 释放pFrame
        av_frame_free(&pFrame);
        av_frame_free(&pFrameYUV);

        // 关闭解码器
        avcodec_close(pCodecCtx);

        // 关闭文件
        avformat_close_input(&pFormatCtx);

        return 0;
    }
};