#include "FFmpegAudioPlayer.h"
#include <iostream>

using namespace std;

FFmpegAudioPlayer::FFmpegAudioPlayer()
{
    initDatas();
}

void FFmpegAudioPlayer::play(const std::string& filePath)
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


}

bool FFmpegAudioPlayer::initDatas()
{
    m_formatContext = avformat_alloc_context();
    m_audioConvertContext = swr_alloc();

    return true;
}

bool FFmpegAudioPlayer::getStreams()
{
    //�������ļ�
    if (avformat_open_input(&m_formatContext, m_filePath.c_str(), nullptr, nullptr) != 0)
    {
        fprintf(stderr, "Could not get input file format\n");
        return false;
    }

    //��ȡ�ļ���Ϣ
    if (avformat_find_stream_info(m_formatContext, NULL) < 0)
    {
        fprintf(stderr, "Could not get input file stream info\n");
        return false;
    }

    // �ҵ���Ƶ��
    for (int i = 0; i < m_formatContext->nb_streams; i++)
    {
        if (m_formatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            m_audioStream = i;
            break;
        }
    }

    return true;
}

bool FFmpegAudioPlayer::avcodecOpen()
{
    m_codecContext = m_formatContext->streams[m_audioStream]->codec;
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
    cout << "Bitrate:\t " << m_formatContext->bit_rate << endl;
    cout << "Decoder Name:\t" << m_codecContext->codec->long_name << endl;
    cout << "Channels:\t " << m_codecContext->channels << endl;
    cout << "Sample per Second : \t" << m_codecContext->sample_rate << endl;
    cout << "audio name : " << m_avCodec->name << endl;

    av_dump_format(m_formatContext, 0, m_filePath.c_str(), 0);
    return true;
}
