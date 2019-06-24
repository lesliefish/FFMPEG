#include "FFmpegQtPlayer.h"
#include <iostream>
#include <QPainter>
#include <QResizeEvent>
#include <QEventLoop>

using namespace std;

FFmpegQtPlayer::FFmpegQtPlayer(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(tr("FFmpeg+Qt Video Player"));

    initDatas();

    // �յ�ˢ����Ϣ ����update()ˢ��
    connect(this, &FFmpegQtPlayer::signalRefresh, [&] { update(); });
}

FFmpegQtPlayer::~FFmpegQtPlayer()
{
    av_free(m_rgbFrame); m_rgbFrame = nullptr;
    av_free(m_frame); m_frame = nullptr;
    avcodec_close(m_codecContext); m_codecContext = nullptr;
    avformat_close_input(&m_formatCtx); m_formatCtx = nullptr;
    m_thread.join();
}

/*
 * @func   FFmpegQtPlayer::play 
 * @brief  ����
 * @param  [in]  const std::string & filPath ��Ƶ�ļ�·�� 
 * @param  [in]  int width ���ڿ�� 
 * @param  [in]  int height  ���ڸ߶�
 * @return void  
 */ 
void FFmpegQtPlayer::play(const std::string& filPath, int width /*= 1280*/, int height /*= 720*/)
{
    m_filePath = filPath;
    m_playerWidth = width;
    m_playerHeight = height;
    resize(m_playerWidth, m_playerHeight);

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

    show();

    bool isReadOver = false;
    QEventLoop loop;
    // �ŵ����߳�ִ��
    m_thread = std::thread([&]
    {
        while (!isReadOver)
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

            m_swsContext = sws_getContext(m_codecContext->width, m_codecContext->height,
                m_codecContext->pix_fmt, m_playerWidth, m_playerHeight, AV_PIX_FMT_RGBA, SWS_BICUBIC, nullptr, nullptr, nullptr);
            if (!m_swsContext)
            {
                continue;
            }

            int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, m_playerWidth, m_playerHeight, 1);
            shared_ptr<uint8_t*> buffer = make_shared<uint8_t*>(new uint8_t[bytes]);
            av_image_fill_arrays(m_rgbFrame->data, m_rgbFrame->linesize, *buffer.get(), AV_PIX_FMT_RGBA, m_playerWidth, m_playerHeight, 1);

            // ��֡����תΪ���Ŵ�����Ҫ�Ĵ�С
            sws_scale(m_swsContext, (const uint8_t* const*)m_frame->data, m_frame->linesize, 0,
                m_codecContext->height, m_rgbFrame->data, m_rgbFrame->linesize);

            QImage tempImage((uchar *)*buffer.get(), m_playerWidth, m_playerHeight, QImage::Format_RGBA8888);
            m_image = std::move(tempImage);

            emit signalRefresh();   //֪ͨˢ����ʾ

            sws_freeContext(m_swsContext);
            av_free_packet(m_avPacket);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        loop.quit();
    });

    loop.exec();// �ȴ��߳�ִ�н���
    close();
}

bool FFmpegQtPlayer::initDatas()
{
    m_avPacket = av_packet_alloc();
    m_formatCtx = avformat_alloc_context();
    m_rgbFrame = av_frame_alloc();
    m_frame = av_frame_alloc();

    return true;
}

/*
 * @func   FFmpegQtPlayer::getStreams 
 * @brief  ��ȡ��Ƶ��
 * @return bool  
 */ 
bool FFmpegQtPlayer::getStreams()
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
            break;
        }
    }

    return true;
}

/*
 * @func   FFmpegQtPlayer::avcodecOpen 
 * @brief  �򿪽�����
 * @return bool  
 */ 
bool FFmpegQtPlayer::avcodecOpen()
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

/*
 * @func   FFmpegQtPlayer::paintEvent 
 * @brief  ��ͼ�¼�
 * @param  [in]  QPaintEvent * event  
 * @return void  
 */ 
void FFmpegQtPlayer::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, width(), height()); //�Ȼ��ɺ�ɫ

    if (m_image.isNull())
        return;

    painter.drawImage(QPoint(0, 0), m_image); //����ͼ��
}

void FFmpegQtPlayer::resizeEvent(QResizeEvent *event)
{
    m_playerWidth = event->size().width();
    m_playerHeight = event->size().height();
    return QWidget::resizeEvent(event);
}
