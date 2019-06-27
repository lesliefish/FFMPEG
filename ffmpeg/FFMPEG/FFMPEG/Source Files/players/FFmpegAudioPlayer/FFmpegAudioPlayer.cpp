#include "FFmpegAudioPlayer.h"
#include <iostream>
#include <memory>
#include <future>

using namespace std;

namespace
{
    constexpr int maxAudioFrameSize = 192000; // 1 second of 48khz 32bit audio
    //Buffer:
    //|-----------|-------------|
    //chunk-------pos---len-----|
    static  Uint8  *audioChunk;
    static  Uint32  audioLen;
    static  Uint8  *audioPos;
}

FFmpegAudioPlayer::FFmpegAudioPlayer()
{
    initDatas();
}

void FFmpegAudioPlayer::play(const std::string& filePath)
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

    // 音频输出
    uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO; // 声道输出立体声
    //AAC:1024  MP3:1152
    int outNumberSamples = m_codecContext->frame_size; // 音频帧中每个频道的采样数。
    AVSampleFormat outSampleFormat = AV_SAMPLE_FMT_S16;
    int outSampleRate = 44100; //44100HZ 采样率
    int outChannels = av_get_channel_layout_nb_channels(outChannelLayout);
    // 输出大小
    int outBufferSize = av_samples_get_buffer_size(NULL, outChannels, outNumberSamples, outSampleFormat, 1);
    shared_ptr<uint8_t*> pOutBuffer = make_shared<uint8_t*>(new uint8_t[maxAudioFrameSize * 2]);

    if (SDL_Init(SDL_INIT_AUDIO))
    {
        fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
        return;
    }
    //SDL_AudioSpec
    SDL_AudioSpec wantedSpec;
    wantedSpec.freq = outSampleRate;
    wantedSpec.format = AUDIO_S16SYS;
    wantedSpec.channels = outChannels;
    wantedSpec.silence = 0;
    wantedSpec.samples = outNumberSamples;
    wantedSpec.callback = [](void *udata, Uint8 *stream, int len)
    {
        if (audioLen == 0)		/*  只有在我们还有数据的时候才播放  */
            return;
        len = (len > audioLen ? audioLen : len);
        SDL_MixAudio(stream, audioPos, len, SDL_MIX_MAXVOLUME);
        audioPos += len;
        audioLen -= len;
    };
    wantedSpec.userdata = m_codecContext;

    if (SDL_OpenAudio(&wantedSpec, NULL) < 0)
    {
        fprintf(stderr, "can't open audio.\n");
        return;
    }

    uint32_t ret, len = 0;
    int got_picture;
    int index = 0;
    //FIX:Some Codec's Context Information is missing
    int64_t inChannelLayout = av_get_default_channel_layout(m_codecContext->channels);

    m_audioConvertContext = swr_alloc_set_opts(m_audioConvertContext, outChannelLayout, outSampleFormat, outSampleRate,
        inChannelLayout, m_codecContext->sample_fmt, m_codecContext->sample_rate, 0, NULL);
    swr_init(m_audioConvertContext);

    //Play
    SDL_PauseAudio(0);

    while (av_read_frame(m_formatContext, m_avPacket) >= 0)
    {
        if (m_avPacket->stream_index == m_audioStream)
        {
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
            swr_convert(m_audioConvertContext, &*pOutBuffer.get(), maxAudioFrameSize, (const uint8_t **)m_frame->data, m_frame->nb_samples);

            printf("index:%5d\t pts:%lld\t packet size:%d\n", index, m_avPacket->pts, m_avPacket->size);

            index++;
            //SDL------------------
            audioChunk = (Uint8 *)*pOutBuffer.get();
            //Audio buffer length
            audioLen = outBufferSize;

            audioPos = audioChunk;

            while (audioLen > 0)//Wait until finish
                SDL_Delay(1);
        }
        av_free_packet(m_avPacket);
    }
}

bool FFmpegAudioPlayer::initDatas()
{
    m_avPacket = av_packet_alloc();
    m_formatContext = avformat_alloc_context();
    m_audioConvertContext = swr_alloc();
    m_frame = av_frame_alloc();
    m_audioConvertContext = swr_alloc();

    return true;
}

bool FFmpegAudioPlayer::getStreams()
{
    //打开输入文件
    if (avformat_open_input(&m_formatContext, m_filePath.c_str(), nullptr, nullptr) != 0)
    {
        fprintf(stderr, "Could not get input file format\n");
        return false;
    }

    //获取文件信息
    if (avformat_find_stream_info(m_formatContext, NULL) < 0)
    {
        fprintf(stderr, "Could not get input file stream info\n");
        return false;
    }

    // 找到音频流
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
    cout << "Bitrate:\t " << m_formatContext->bit_rate << endl;
    cout << "Decoder Name:\t" << m_codecContext->codec->long_name << endl;
    cout << "Channels:\t " << m_codecContext->channels << endl;
    cout << "Sample per Second : \t" << m_codecContext->sample_rate << endl;
    cout << "Audio name : " << m_avCodec->name << endl;

    av_dump_format(m_formatContext, 0, m_filePath.c_str(), 0);
    return true;
}
