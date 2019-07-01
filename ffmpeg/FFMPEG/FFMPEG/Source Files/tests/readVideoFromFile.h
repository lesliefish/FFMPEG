/** @file   readVideoFromFile.h
 *  @brief  һ��Сʾ��������ʾ���ʹ��libavformat��libavcodec���ļ��ж�ȡ��Ƶ��
 *  @note   update from http://dranger.com/ffmpeg/tutorial01.c
 *  @author lesliefish
 *  @date   2019/06/27
 */

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include <stdio.h>

 // compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

void saveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int  y;

    // ���ļ�
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;

    // д���ļ�ͷ
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // д����������
    for (y = 0; y < height; y++)
    {
        fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, pFile);
    }

    // �ر��ļ�
    fclose(pFile);
}

int main(int argc, char *argv[]) 
{
    // ����Щֵ��ʼ��Ϊnull
    AVFormatContext   *pFormatCtx = NULL;
    int               i, videoStream;
    AVCodecContext    *pCodecCtxOrig = NULL;
    AVCodecContext    *pCodecCtx = NULL;
    AVCodec           *pCodec = NULL;
    AVFrame           *pFrame = NULL;
    AVFrame           *pFrameRGB = NULL;
    AVPacket          packet;
    int               frameFinished;
    int               numBytes;
    uint8_t           *buffer = NULL;
    struct SwsContext *sws_ctx = NULL;

    // ��Ƶ�ļ�·��
    char* filePath = "test.mp4";

    // ����Ƶ�ļ�·��
    if (avformat_open_input(&pFormatCtx, filePath, NULL, NULL) != 0)
        return -1; // ��ʧ��

    // ��������Ϣ
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1; // �Ҳ�������Ϣ

    // ���й��ļ�����Ϣ��ӡ����
    av_dump_format(pFormatCtx, 0, filePath, 0);

    // �ҵ���Ƶ��id
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

    // ��ȡָ����Ƶ���ı�����������ĵ�ָ��
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    // ������Ƶ���Ľ�����
    pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if (pCodec == NULL) 
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // �Ҳ���������
    }

    // ���Ʊ�����������
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    av_codec_set_pkt_timebase(pCodecCtx, pFormatCtx->streams[videoStream]->time_base);

    // �򿪽�����
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1; // ��ʧ��

    // ������Ƶ֡����
    pFrame = av_frame_alloc();

    // ����֡����
    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
        return -1;

    // ȷ������Ļ�������С�����仺���� AV_PIX_FMT_RGB24��ʽ Դ��Ƶ֡ͼ�����ʱ����Ļ�������С
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    // �������������pFrameRGB���������ͼƬ����
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    // ��ʼ��SWS context���Žṹ ���ڿ�������ͼ������
    sws_ctx = sws_getContext(pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        AV_PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );

    // ��ȡ֡����ǰ��֡���浽����
    i = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0) 
    {
        // packet�Ƿ�Ϊ��Ƶ��
        if (packet.stream_index == videoStream) 
        {
            // ������Ƶ֡����
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // �Ƿ��ȡ����Ƶ֡����?
            if (frameFinished) 
            {
                // ��ͼƬԭʼ��ʽ����ת��ΪRGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height,
                    pFrameRGB->data, pFrameRGB->linesize);

                // ����֡���ݵ�����(����ֻ����5֡)
                if (++i <= 5)
                {
                    saveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
                }
            }
        }

        // �ͷ�av_read_frame��ȡ��������ʱ����
        av_free_packet(&packet);
    }

    // �ͷ� RGB �ڴ�
    av_free(buffer);
    av_frame_free(&pFrameRGB);

    // �ͷ� YUV �ڴ�
    av_frame_free(&pFrame);

    // �رս�����
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // �ر���Ƶ�ļ�
    avformat_close_input(&pFormatCtx);

    return 0;
}