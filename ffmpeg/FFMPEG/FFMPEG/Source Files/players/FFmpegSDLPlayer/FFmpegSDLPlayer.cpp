#include "FFmpegSDLPlayer.h"
#include <iostream>
#include <memory>

using namespace std;

FFmpegSDLPlayer::FFmpegSDLPlayer()
{
    initDatas();
}

void FFmpegSDLPlayer::play(const std::string& filePath)
{
    m_filePath = filePath;
    if (!getStreams())
    {
        return;
    }

    if (!avcodecOpen())
    {
        return;
    }

    if (!initSdlInfo())
    {
        return;
    }

    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, m_codecContext->width, m_codecContext->height);
    shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
    avpicture_fill((AVPicture *)m_yuvFrame, *buffer.get(), AV_PIX_FMT_YUV420P, m_codecContext->width, m_codecContext->height);//填充AVFrame
    SDL_Rect rect;

    while (av_read_frame(m_formatCtx, m_avPacket) >= 0)
    {
        if (m_avPacket->stream_index == m_videoStream)
        {
            static int i = 1;
            cout << i++ << endl;
            int ret = avcodec_send_packet(m_codecContext, m_avPacket);
            if (ret < 0)
            {
                fprintf(stderr, "Error sending a packet for decoding\n");
                return;
            }

            while (ret >= 0)
            {
                ret = avcodec_receive_frame(m_codecContext, m_yuvFrame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    continue;
                }
                else if (ret < 0)
                {
                    fprintf(stderr, "Error during decoding\n");
                    continue;
                }

                if (ret >= 0)
                {
                    // SDL绘制处理
                    m_swsContext = sws_getContext(m_codecContext->width, m_codecContext->height,
                        m_codecContext->pix_fmt, m_codecContext->width, m_codecContext->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                        nullptr, nullptr, nullptr);
                    if (!m_swsContext)
                    {
                        continue;
                    }

                    sws_scale(m_swsContext, (const uint8_t* const*)m_yuvFrame->data, m_yuvFrame->linesize, 0,
                        m_codecContext->height, m_yuvFrame->data, m_yuvFrame->linesize);
                    sws_freeContext(m_swsContext);

                    rect.x = 0;
                    rect.y = 0;
                    rect.w = m_codecContext->width;
                    rect.h = m_codecContext->height;

                    SDL_UpdateTexture(m_sdlTexture, &rect, m_yuvFrame->data[0], m_yuvFrame->linesize[0]);
                    SDL_RenderClear(m_sdlRenderer);
                    SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, &rect, &rect);
                    SDL_RenderPresent(m_sdlRenderer);
                    //延时20ms
                    SDL_Delay(20);
                }
            }
        }
    }
}

bool FFmpegSDLPlayer::initDatas()
{
    m_avPacket = av_packet_alloc();
    m_formatCtx = avformat_alloc_context();
    m_yuvFrame = av_frame_alloc();

    return true;
}

bool FFmpegSDLPlayer::initSdlInfo()
{
    if (SDL_Init(SDL_INIT_VIDEO))
    {
        cout << "Could not initialize SDL : " << SDL_GetError() << endl;
        return false;
    }
    // 播放窗口创建
    m_playerWindow = SDL_CreateWindow("FFmpeg SDL2 Player",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_codecContext->width, m_codecContext->height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

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
    m_sdlTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_YV12, SDL_TEXTUREACCESS_STREAMING, m_codecContext->width, m_codecContext->height);
    if (!m_sdlTexture)
    {
        cout << "SDL: could not create texture - exiting: " << SDL_GetError() << endl;
        return false;
    }

    return true;
}

bool FFmpegSDLPlayer::getStreams()
{
    //打开输入视频文件
    if (avformat_open_input(&m_formatCtx, m_filePath.c_str(), nullptr, nullptr) != 0)
    {
        fprintf(stderr, "Could not get input file format\n");
        return false;
    }

    //获取视频文件信息
    if (avformat_find_stream_info(m_formatCtx, NULL) < 0)
    {
        fprintf(stderr, "Could not get input file stream info\n");
        return false;
    }

    // 找到音频流和视频流
    for (int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        //流的类型
        if (m_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoStream = i;
        }
        else if (m_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_audioStream = i;
        }
    }

    return true;
}

bool FFmpegSDLPlayer::avcodecOpen()
{
    m_codecContext = m_formatCtx->streams[m_videoStream]->codec;
    // 寻找解码器
    m_avCodec = avcodec_find_decoder(m_codecContext->codec_id);

    if (!m_avCodec)
    {
        return false;
    }

    m_codecParserContext = av_parser_init(m_avCodec->id);
    if (!m_codecParserContext)
    {
        return false;
    }

    // 打开解码器
    if (avcodec_open2(m_codecContext, m_avCodec, nullptr) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        return false;
    }
    
    //输出视频信息
    cout << "video format ： " << m_formatCtx->iformat->name << endl;
    cout << "video duration ： " << m_formatCtx->duration / 1000000 << endl;
    cout << "video width * height : " << m_codecContext->width << " * " << m_codecContext->height << endl;
    cout << "video name : " << m_avCodec->name << endl;

    av_dump_format(m_formatCtx, 0, m_filePath.c_str(), 0);
    return true;
}
