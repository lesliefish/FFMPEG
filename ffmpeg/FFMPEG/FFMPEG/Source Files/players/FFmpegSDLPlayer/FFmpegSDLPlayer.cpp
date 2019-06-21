#include "FFmpegSDLPlayer.h"
#include <iostream>
#include <memory>
#include <future>

using namespace std;

// 自定义刷新事件
#define REFRESH_EVENT  (SDL_USEREVENT + 1)


/*
 * @func   FFmpegSDLPlayer::FFmpegSDLPlayer 
 * @brief  构造函数
 * @param  [in]  const std::string & playerName  播放器名称
 * @param  [in]  int playWindowWidth  初始播放器宽度
 * @param  [in]  int playWindowHeight  初始播放器高度
 * @return   
 */ 
FFmpegSDLPlayer::FFmpegSDLPlayer(const std::string& playerName, int playWindowWidth /*= 1280*/, int playWindowHeight /*= 720*/)
{
    initDatas();
    m_playerWidth = playWindowWidth;
    m_playerHeight = playWindowHeight;
    m_playerName = playerName;
}

FFmpegSDLPlayer::~FFmpegSDLPlayer()
{
    SDL_DestroyWindow(m_playerWindow);
    SDL_DestroyTexture(m_sdlTexture);
    SDL_Quit();
    av_free(m_yuvFrame); m_yuvFrame = nullptr;
    av_free(m_frame); m_frame = nullptr;
    avcodec_close(m_codecContext); m_codecContext = nullptr;
    avformat_close_input(&m_formatCtx); m_formatCtx = nullptr;
}

/*
 * @func   FFmpegSDLPlayer::play 
 * @brief  播放视频文件
 * @param  [in]  const std::string & filePath  
 * @return void  
 */ 
void FFmpegSDLPlayer::play(const std::string& filePath)
{
    m_filePath = filePath;
    // 获取流
    if (!getStreams())
    {
        return;
    }

    // 打开解码器
    if (!avcodecOpen())
    {
        return;
    }

    // 初始化SDL播放窗口
    if (!initSdlInfo())
    {
        return;
    }

    // 一帧AV_PIX_FMT_YUV420P格式图像size
    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);
    shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
    avpicture_fill((AVPicture *)m_yuvFrame, *buffer.get(), AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);//填充AVFrame
    SDL_Rect rect;
    SDL_Event event;

    // 是否读数据完毕
    bool isReadOver{ false };

    // 线程处理部分 每隔40ms读取一帧视频数据
    auto fut = std::async([&]
    {
        while (!isReadOver)
        {
            SDL_Event event;
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
            SDL_Delay(40); // 40ms取一帧数据, 刷新一次
        }

        // 读完数据后发送关闭消息
        SDL_Event event;
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    });

    for (;;)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT)
        {
            // 退出事件
            isReadOver = true;
            break;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            // 主要用来检测窗口是否发生变化
            int curWidth{ 0 };
            int curHeight{ 0 };
            SDL_GetWindowSize(m_playerWindow, &curWidth, &curHeight);
            if (curWidth != m_playerWidth || curHeight != m_playerHeight)
            {
                // 仅当尺寸变了时才执行如下语句
                m_playerWidth = curWidth;
                m_playerHeight = curHeight;
                int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);
                shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
                avpicture_fill((AVPicture *)m_yuvFrame, *buffer.get(), AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);//填充AVFrame
            }
        }
        else if (event.type == REFRESH_EVENT)
        {
            bool findVideoStream{ false }; // 是否读到视频流
            while (!findVideoStream)
            {
                // 一直读数据 直到读到视频流为止
                if (av_read_frame(m_formatCtx, m_avPacket) < 0)
                {
                    isReadOver = true;
                    break;
                }

                if (m_avPacket->stream_index != m_videoStream)
                {
                    continue;
                }
                findVideoStream = true;
            }

            int ret = avcodec_send_packet(m_codecContext, m_avPacket);
            if (ret < 0)
            {
                fprintf(stderr, "Error sending a packet for decoding\n");
                continue;
            }

            ret = avcodec_receive_frame(m_codecContext, m_frame);
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
            m_swsContext = sws_getContext(m_codecContext->width, m_codecContext->height,
                m_codecContext->pix_fmt, m_playerWidth, m_playerHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                nullptr, nullptr, nullptr);
            if (!m_swsContext)
            {
                continue;
            }
            
            // 将帧数据转为播放窗口需要的大小
            sws_scale(m_swsContext, (const uint8_t* const*)m_frame->data, m_frame->linesize, 0,
                m_codecContext->height, m_yuvFrame->data, m_yuvFrame->linesize);
            sws_freeContext(m_swsContext);

            rect.x = 0;
            rect.y = 0;
            rect.w = m_playerWidth;
            rect.h = m_playerHeight;

            SDL_UpdateTexture(m_sdlTexture, &rect, m_yuvFrame->data[0], m_yuvFrame->linesize[0]);
            SDL_RenderClear(m_sdlRenderer);
            SDL_RenderCopy(m_sdlRenderer, m_sdlTexture, &rect, &rect);
            SDL_RenderPresent(m_sdlRenderer);
            av_free_packet(m_avPacket);
        }
    }
}

bool FFmpegSDLPlayer::initDatas()
{
    m_avPacket = av_packet_alloc();
    m_formatCtx = avformat_alloc_context();
    m_yuvFrame = av_frame_alloc();
    m_frame = av_frame_alloc();

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
    m_playerWindow = SDL_CreateWindow(m_playerName.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_playerWidth, m_playerHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

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
