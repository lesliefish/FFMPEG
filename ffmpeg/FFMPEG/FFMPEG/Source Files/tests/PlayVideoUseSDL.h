/** @file   PlayVideoUseSDL.h
 *
 *  @brief  ʹ��SDL2.0������Ƶ
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
        // ����Ƶ�ļ�
        if (avformat_open_input(&pFormatCtx, pFilePath, NULL, NULL) != 0)
            return -1; // ��ʧ��

        // ������Ϣ
        if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
            return -1; // �Ҳ�����

        // ��Ƶ�ļ���Ϣ
        av_dump_format(pFormatCtx, 0, pFilePath, 0);

        // �ҵ���Ƶ��
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
            return -1; // δ�ҵ���Ƶ��

        // Get a pointer to the codec context for the video stream
        pCodecCtx = pFormatCtx->streams[videoStream]->codec;
        // ��Ƶ���Ľ�����
        pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
        if (pCodec == NULL)
        {
            fprintf(stderr, "Unsupported codec!\n");
            return -1; // ������δ�ҵ�
        }

        // �򿪽�����
        if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
            return -1; // ʧ��

        // ������Ƶ֡����
        pFrame = av_frame_alloc();
        // ����֡����
        pFrameYUV = av_frame_alloc();

        //���Ŵ��ڴ���
        playerWindow = SDL_CreateWindow("SDL 2.0", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (!playerWindow)
        {
            fprintf(stderr, "SDL: create window falied.\n");
            exit(1);
        }

        // ��Ⱦ
        sdlRenderer = SDL_CreateRenderer(playerWindow, -1, 0);
        if (!sdlRenderer)
        {
            fprintf(stderr, "SDL: could not create render - exiting: \n");
            return false;
        }

        // ����
        sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);
        if (!sdlTexture)
        {
            fprintf(stderr, "SDL: could not create texture - exiting: \n");
            return false;
        }

        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
        shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
        av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, *buffer.get(), AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

        // �Ƿ���������
        bool isReadOver{ false };

        // �̴߳����� ÿ��40ms��ȡһ֡��Ƶ����
        auto fut = std::async([&]
        {
            SDL_Event event;
            while (!isReadOver)
            {
                event.type = REFRESH_EVENT;
                SDL_PushEvent(&event);
                SDL_Delay(40); // 40msȡһ֡����, ˢ��һ��
            }

            // �������ݺ��͹ر���Ϣ
            event.type = SDL_QUIT;
            SDL_PushEvent(&event);
        });

        // �¼�����
        for (;;)
        {
            SDL_WaitEvent(&event);
            if (event.type == REFRESH_EVENT)
            {
                // ��֡���ݲ�����
                if (av_read_frame(pFormatCtx, &packet) >= 0)
                {
                    // �Ƿ���Ƶ��packet?
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

                        // ��ʼ��SWS 
                        sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                            pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                            nullptr, nullptr, nullptr);
                        // ��֡����תΪ���Ŵ�����Ҫ�Ĵ�С
                        sws_scale(sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                            pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
                        // �ͷ�av_read_frame�����packet
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
                        // ÿ�λ�ȡ�Ĳ�����Ƶ�� �ͻ���ɿ���40ms ��Ҫ����ȥ��ȡ��һ֡��Ƶ
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

        // �ͷ�pFrame
        av_frame_free(&pFrame);
        av_frame_free(&pFrameYUV);

        // �رս�����
        avcodec_close(pCodecCtx);

        // �ر��ļ�
        avformat_close_input(&pFormatCtx);

        return 0;
    }
};