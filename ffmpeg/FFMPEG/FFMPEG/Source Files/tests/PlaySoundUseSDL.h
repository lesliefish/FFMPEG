/** @file   PlaySoundUseSDL.h
 *  @brief  播放音频 使用SDL2.0
 *  @note   updates from https://github.com/chelyaev/ffmpeg-tutorial
 *  @author lesliefish
 *  @date   2019/07/02
 */
#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
}

#include <SDL.h>
#include <SDL_thread.h>

#include <stdio.h>
#include <memory>
#include <assert.h>

using namespace std;
// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000

typedef struct PacketQueue 
{
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;

PacketQueue audioQueue;

int quit = 0;

void packetQueueInit(PacketQueue *q) 
{
    memset(q, 0, sizeof(PacketQueue));
    q->mutex = SDL_CreateMutex();
    q->cond = SDL_CreateCond();
}

int packetQueuePut(PacketQueue *q, AVPacket *pkt) 
{
    AVPacketList *pkt1;
    if (av_dup_packet(pkt) < 0) 
    {
        return -1;
    }
    pkt1 = (AVPacketList *)av_malloc(sizeof(AVPacketList));
    if (!pkt1)
        return -1;
    pkt1->pkt = *pkt;
    pkt1->next = NULL;


    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)
        q->first_pkt = pkt1;
    else
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);
    return 0;
}

static int packetQueueGet(PacketQueue *q, AVPacket *pkt, int block)
{
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;) 
    {
        if (quit) 
        {
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1) 
        {
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            *pkt = pkt1->pkt;
            av_free(pkt1);
            ret = 1;
            break;
        }
        else if (!block) {
            ret = 0;
            break;
        }
        else {
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}

int audioDecodeFrame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size) {
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;
    static AVFrame frame;

    int len1, data_size = 0;

    for (;;)
    {
        while (audio_pkt_size > 0) 
        {
            int got_frame = 0;
            len1 = avcodec_decode_audio4(aCodecCtx, &frame, &got_frame, &pkt);
            if (len1 < 0) 
            {
                /* if error, skip frame */
                audio_pkt_size = 0;
                break;
            }
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            data_size = 0;
            if (got_frame) 
            {
                data_size = av_samples_get_buffer_size(NULL,
                    aCodecCtx->channels,
                    frame.nb_samples,
                    aCodecCtx->sample_fmt,
                    1);
                assert(data_size <= buf_size);
                memcpy(audio_buf, frame.data[0], data_size);
            }
            if (data_size <= 0) {
                /* No data yet, get more frames */
                continue;
            }
            /* We have data, return it and come back for more later */
            return data_size;
        }
        if (pkt.data)
            av_free_packet(&pkt);

        if (quit) {
            return -1;
        }

        if (packetQueueGet(&audioQueue, &pkt, 1) < 0) {
            return -1;
        }
        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}

void audioCallback(void *userdata, Uint8 *stream, int len) 
{
    AVCodecContext *aCodecCtx = (AVCodecContext *)userdata;
    int len1, audio_size;

    static uint8_t audioBuf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while (len > 0) 
    {
        if (audio_buf_index >= audio_buf_size) 
        {
            /* We have already sent all our data; get more */
            audio_size = audioDecodeFrame(aCodecCtx, audioBuf, sizeof(audioBuf));
            if (audio_size < 0) 
            {
                /* If error, output silence */
                audio_buf_size = 1024; // arbitrary?
                memset(audioBuf, 0, audio_buf_size);
            }
            else 
            {
                audio_buf_size = audio_size;
            }
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if (len1 > len)
            len1 = len;
        memcpy(stream, (uint8_t *)audioBuf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}

int main(int argc, char *argv[]) 
{
    AVFormatContext *pFormatCtx = NULL;
    int             i, videoStream, audioStream;
    AVCodecContext  *pCodecCtx = NULL;
    AVCodec         *pCodec = NULL;
    AVFrame         *pFrame = NULL;
    AVFrame         *pFrameRGB = NULL;
    AVPacket        packet;
    int             frameFinished;
    struct SwsContext *sws_ctx = NULL;

    AVCodecContext  *aCodecCtx = NULL;
    AVCodec         *aCodec = NULL;

    SDL_Window* playerWindow = NULL;
    SDL_Renderer* sdlRenderer = NULL;
    SDL_Texture* sdlTexture = NULL;
    SDL_Rect        rect;
    SDL_Event       event;

    SDL_AudioSpec   want, have;
    
    const char* pFilePath = "Wildlife.wmv";

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) 
    {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        exit(1);
    }

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
    audioStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) 
    {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
            videoStream < 0) 
        {
            videoStream = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
            audioStream < 0) 
        {
            audioStream = i;
        }
    }
    if (videoStream == -1)
        return -1; // Didn't find a video stream
    if (audioStream == -1)
        return -1;

    aCodecCtx = pFormatCtx->streams[audioStream]->codec;
    aCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (!aCodec) {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    // Set audio settings from codec info
    want.freq = aCodecCtx->sample_rate;
    want.format = AUDIO_S16SYS;
    want.channels = aCodecCtx->channels;
    want.silence = 0;
    want.samples = SDL_AUDIO_BUFFER_SIZE;
    want.callback = audioCallback;
    want.userdata = aCodecCtx;

    if (SDL_OpenAudio(&want, &have) < 0) 
    {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        return -1;
    }

    avcodec_open2(aCodecCtx, aCodec, NULL);

    // audio_st = pFormatCtx->streams[index]
    packetQueueInit(&audioQueue);
    SDL_PauseAudio(0);

    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) 
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // Codec not found
    }

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1; // Could not open codec

                   // Allocate video frame
    pFrame = av_frame_alloc();
    pFrameRGB = av_frame_alloc();

    // Make a screen to put our video
    playerWindow = SDL_CreateWindow("Audio Player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        pCodecCtx->width, pCodecCtx->height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    // 渲染
    sdlRenderer = SDL_CreateRenderer(playerWindow, -1, 0);
    // 纹理
    sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_ARGB32, SDL_TEXTUREACCESS_STREAMING, pCodecCtx->width, pCodecCtx->height);

    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_ARGB, pCodecCtx->width, pCodecCtx->height, 1);
    shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
    av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, *buffer.get(), AV_PIX_FMT_ARGB, pCodecCtx->width, pCodecCtx->height, 1);

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

            // 初始化SWS 
            sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_ARGB, SWS_BICUBIC,
                nullptr, nullptr, nullptr);
            // 将帧数据转为播放窗口需要的大小
            sws_scale(sws_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0,
                pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
            // 释放av_read_frame申请的packet
            sws_freeContext(sws_ctx);

            rect.x = 0;
            rect.y = 0;
            rect.w = pCodecCtx->width;
            rect.h = pCodecCtx->height;
            SDL_UpdateTexture(sdlTexture, &rect, pFrameRGB->data[0], pFrameRGB->linesize[0]);
            SDL_RenderClear(sdlRenderer);
            SDL_RenderCopy(sdlRenderer, sdlTexture, &rect, &rect);
            SDL_RenderPresent(sdlRenderer);
            av_free_packet(&packet);
        }
        else if (packet.stream_index == audioStream) 
        {
            packetQueuePut(&audioQueue, &packet);
        }
        else 
        {
            av_free_packet(&packet);
        }
        // Free the packet that was allocated by av_read_frame
        SDL_PollEvent(&event);
        switch (event.type) 
        {
        case SDL_QUIT:
            quit = 1;
            SDL_Quit();
            exit(0);
            break;
        default:
            break;
        }

    }

    // Free the YUV frame
    av_frame_free(&pFrame);
    av_frame_free(&pFrameRGB);

    // Close the codecs
    avcodec_close(pCodecCtx);
    avcodec_close(aCodecCtx);

    // Close the video file
    avformat_close_input(&pFormatCtx);

    return 0;
}