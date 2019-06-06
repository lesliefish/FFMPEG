#include "VideoDecoder.h"
#include <iostream>

#define INBUF_SIZE 4096

VideoDecoder::VideoDecoder()
{
    m_avPacket = av_packet_alloc();
    m_formatCtx = avformat_alloc_context();
    m_frame = av_frame_alloc();
}

VideoDecoder::~VideoDecoder()
{
    av_parser_close(m_parser);
    avcodec_free_context(&m_context);
    av_frame_free(&m_frame);
    av_packet_free(&m_avPacket);
}

void VideoDecoder::exec(const string& in, const string& out)
{
    VideoDecoder decoder;
    decoder.doDecode(in, out);
}

/*
 * @func   VideoDecoder::doDecode
 * @brief  ��Ƶ����
 * @param  [in]  const string & in
 * @param  [in]  const string & out
 * @return void
 */
void VideoDecoder::doDecode(const string& in, const string& out)
{
    FILE* pFile{ nullptr };
    size_t dataSize{ 0 };
    uint8_t inbuf[INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE];
    uint8_t* data = inbuf;

    int videoStreamId = getVideoStreamId(in);
    if (-1 == videoStreamId)
    {
        return;
    }

    // ��ȡ��Ƶ���еı���������� ֻ��֪����Ƶ�ı��뷽ʽ�����ܹ����ݱ��뷽ʽȥ�ҵ������� // https://blog.csdn.net/zwz1984/article/details/82824524
    AVCodecContext *pCodecCtx = m_formatCtx->streams[videoStreamId]->codec;
    m_avCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    m_parser = av_parser_init(m_avCodec->id);
    m_context = avcodec_alloc_context3(m_avCodec);
    // �򿪽�����
    avcodec_open2(m_context, m_avCodec, nullptr);

    //�����Ƶ��Ϣ
    cout << "��Ƶ���ļ���ʽ��" << m_formatCtx->iformat->name << endl;
    cout << "��Ƶʱ����" << m_formatCtx->duration / 1000000 << endl;
    cout << "��Ƶ�Ŀ�ߣ�" << pCodecCtx->width << pCodecCtx->height << endl;
    cout << "�����������ƣ�" << m_avCodec->name << endl;

    pFile = fopen(in.c_str(), "rb");
    while (!feof(pFile)) 
    {
        /* read raw data from the input file */
        dataSize = fread(inbuf, 1, INBUF_SIZE, pFile);
        if (!dataSize)
            break;

        /* use the parser to split the data into frames */
        while (dataSize > 0)
        {
            int len = av_parser_parse2(m_parser, m_context, &m_avPacket->data, &m_avPacket->size, inbuf, dataSize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
            if (len < 0)
            {
                fprintf(stderr, "Error while parsing\n");
                return;
            }
            data += len;
            dataSize -= len;

            if (m_avPacket->size)
            {
                decode(m_context, m_frame, m_avPacket, out);
            }
        }
    }

    /* flush the decoder */
    decode(m_context, m_frame, NULL, out);
    fclose(pFile);
}

/*
 * @func   VideoDecoder::decode
 * @brief  ����
 * @param  [in]  AVCodecContext * dec_ctx
 * @param  [in]  AVFrame * frame
 * @param  [in]  AVPacket * pkt
 * @param  [in] const std::string & fileName
 * @return void
 */
void VideoDecoder::decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const std::string& fileName)
{
    int ret;

    ret = avcodec_send_packet(dec_ctx, pkt);
    if (ret < 0) 
    {
        fprintf(stderr, "Error sending a packet for decoding\n");
        exit(1);
    }

    while (ret >= 0)
    {
        ret = avcodec_receive_frame(dec_ctx, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return;
        }
        else if (ret < 0)
        {
            fprintf(stderr, "Error during decoding\n");
            exit(1);
        }

        printf("saving frame %3d\n", dec_ctx->frame_number);
        fflush(stdout);

        /* the picture is allocated by the decoder. no need to free it */
        string newFileName = fileName + string("_") + to_string(dec_ctx->frame_number);
        pgmSave(frame->data[0], frame->linesize[0], frame->width, frame->height, newFileName);
    }
}

/*
 * @func   VideoDecoder::pgmSave 
 * @brief  ͼƬ����
 * @param  [in]  unsigned char * buf  
 * @param  [in]  int wrap  
 * @param  [in]  int xsize  
 * @param  [in]  int ysize  
 * @param  [in]  const string & filename  
 * @return void  
 */ 
void VideoDecoder::pgmSave(unsigned char *buf, int wrap, int xsize, int ysize, const string& filename)
{
    FILE *pFile{ nullptr };
    pFile = fopen(filename.c_str(), "w");
    fprintf(pFile, "P5\n%d %d\n%d\n", xsize, ysize, 255);
    for (int i = 0; i < ysize; i++)
    {
        fwrite(buf + i * wrap, 1, xsize, pFile);
    }
    fclose(pFile);
}

/*
 * @func   VideoDecoder::getVideoInfo 
 * @brief  ��ȡ��Ƶ��id
 * @param  [in]  const string & inputFile  
 * @return int  
 */ 
int VideoDecoder::getVideoStreamId(const string& inputFile)
{
    //��������Ƶ�ļ�
    if (avformat_open_input(&m_formatCtx, inputFile.c_str(), nullptr, nullptr) != 0)
    {
        fprintf(stderr, "Could not get input file format\n");
        return -1;
    }

    //��ȡ��Ƶ�ļ���Ϣ
    if (avformat_find_stream_info(m_formatCtx, NULL) < 0)
    {
        fprintf(stderr, "Could not get input file stream info\n");
        return -1;
    }

    //�����������͵�������Ƶ������Ƶ������Ļ�������ҵ���Ƶ��
    int streamIndex = -1;
    //nb_streams: number of streams ���ĸ���
    for (int i = 0; i < m_formatCtx->nb_streams; i++)
    {
        //��������
        if (m_formatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            streamIndex = i;
            break;
        }
    }

    if (-1 == streamIndex)
    {
        fprintf(stderr, "Could not find video stream\n");
        return false;
    }

    return streamIndex;
}
