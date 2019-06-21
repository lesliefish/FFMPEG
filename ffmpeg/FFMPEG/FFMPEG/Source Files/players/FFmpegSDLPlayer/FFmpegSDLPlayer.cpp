#include "FFmpegSDLPlayer.h"
#include <iostream>
#include <memory>
#include <future>

using namespace std;

// �Զ���ˢ���¼�
#define REFRESH_EVENT  (SDL_USEREVENT + 1)


/*
 * @func   FFmpegSDLPlayer::FFmpegSDLPlayer 
 * @brief  ���캯��
 * @param  [in]  const std::string & playerName  ����������
 * @param  [in]  int playWindowWidth  ��ʼ���������
 * @param  [in]  int playWindowHeight  ��ʼ�������߶�
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
 * @brief  ������Ƶ�ļ�
 * @param  [in]  const std::string & filePath  
 * @return void  
 */ 
void FFmpegSDLPlayer::play(const std::string& filePath)
{
    m_filePath = filePath;
    // ��ȡ��
    if (!getStreams())
    {
        return;
    }

    // �򿪽�����
    if (!avcodecOpen())
    {
        return;
    }

    // ��ʼ��SDL���Ŵ���
    if (!initSdlInfo())
    {
        return;
    }

    // һ֡AV_PIX_FMT_YUV420P��ʽͼ��size
    int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);
    shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
    avpicture_fill((AVPicture *)m_yuvFrame, *buffer.get(), AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);//���AVFrame
    SDL_Rect rect;
    SDL_Event event;

    // �Ƿ���������
    bool isReadOver{ false };

    // �̴߳����� ÿ��40ms��ȡһ֡��Ƶ����
    auto fut = std::async([&]
    {
        while (!isReadOver)
        {
            SDL_Event event;
            event.type = REFRESH_EVENT;
            SDL_PushEvent(&event);
            SDL_Delay(40); // 40msȡһ֡����, ˢ��һ��
        }

        // �������ݺ��͹ر���Ϣ
        SDL_Event event;
        event.type = SDL_QUIT;
        SDL_PushEvent(&event);
    });

    for (;;)
    {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT)
        {
            // �˳��¼�
            isReadOver = true;
            break;
        }
        else if (event.type == SDL_WINDOWEVENT)
        {
            // ��Ҫ������ⴰ���Ƿ����仯
            int curWidth{ 0 };
            int curHeight{ 0 };
            SDL_GetWindowSize(m_playerWindow, &curWidth, &curHeight);
            if (curWidth != m_playerWidth || curHeight != m_playerHeight)
            {
                // �����ߴ����ʱ��ִ���������
                m_playerWidth = curWidth;
                m_playerHeight = curHeight;
                int numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);
                shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[numBytes]);
                avpicture_fill((AVPicture *)m_yuvFrame, *buffer.get(), AV_PIX_FMT_YUV420P, m_playerWidth, m_playerHeight);//���AVFrame
            }
        }
        else if (event.type == REFRESH_EVENT)
        {
            bool findVideoStream{ false }; // �Ƿ������Ƶ��
            while (!findVideoStream)
            {
                // һֱ������ ֱ��������Ƶ��Ϊֹ
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

            // SDL���ƴ��� 
            m_swsContext = sws_getContext(m_codecContext->width, m_codecContext->height,
                m_codecContext->pix_fmt, m_playerWidth, m_playerHeight, AV_PIX_FMT_YUV420P, SWS_BICUBIC,
                nullptr, nullptr, nullptr);
            if (!m_swsContext)
            {
                continue;
            }
            
            // ��֡����תΪ���Ŵ�����Ҫ�Ĵ�С
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
    // ���Ŵ��ڴ���
    m_playerWindow = SDL_CreateWindow(m_playerName.c_str(),
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        m_playerWidth, m_playerHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

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
    //��������Ƶ�ļ�
    if (avformat_open_input(&m_formatCtx, m_filePath.c_str(), nullptr, nullptr) != 0)
    {
        fprintf(stderr, "Could not get input file format\n");
        return false;
    }

    //��ȡ��Ƶ�ļ���Ϣ
    if (avformat_find_stream_info(m_formatCtx, NULL) < 0)
    {
        fprintf(stderr, "Could not get input file stream info\n");
        return false;
    }

    // �ҵ���Ƶ������Ƶ��
    for (int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        //��������
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
    // Ѱ�ҽ�����
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

    // �򿪽�����
    if (avcodec_open2(m_codecContext, m_avCodec, nullptr) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        return false;
    }

    //�����Ƶ��Ϣ
    cout << "video format �� " << m_formatCtx->iformat->name << endl;
    cout << "video duration �� " << m_formatCtx->duration / 1000000 << endl;
    cout << "video width * height : " << m_codecContext->width << " * " << m_codecContext->height << endl;
    cout << "video name : " << m_avCodec->name << endl;

    av_dump_format(m_formatCtx, 0, m_filePath.c_str(), 0);
    return true;
}
