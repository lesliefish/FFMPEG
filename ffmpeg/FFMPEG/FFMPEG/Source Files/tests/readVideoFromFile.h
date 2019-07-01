/** @file   readVideoFromFile.h
 *  @brief  一个小示例程序，演示如何使用libavformat和libavcodec从文件中读取视频。
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

    // 打开文件
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
        return;

    // 写入文件头
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // 写入像素数据
    for (y = 0; y < height; y++)
    {
        fwrite(pFrame->data[0] + y*pFrame->linesize[0], 1, width * 3, pFile);
    }

    // 关闭文件
    fclose(pFile);
}

int main(int argc, char *argv[]) 
{
    // 将这些值初始化为null
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

    // 视频文件路径
    char* filePath = "test.mp4";

    // 打开视频文件路径
    if (avformat_open_input(&pFormatCtx, filePath, NULL, NULL) != 0)
        return -1; // 打开失败

    // 检索流信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1; // 找不到流信息

    // 将有关文件的信息打印出来
    av_dump_format(pFormatCtx, 0, filePath, 0);

    // 找到视频流id
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
        return -1; // 未找到视频流

    // 获取指向视频流的编解码器上下文的指针
    pCodecCtxOrig = pFormatCtx->streams[videoStream]->codec;
    // 查找视频流的解码器
    pCodec = avcodec_find_decoder(pCodecCtxOrig->codec_id);
    if (pCodec == NULL) 
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1; // 找不到解码器
    }

    // 复制编码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoStream]->codecpar);
    av_codec_set_pkt_timebase(pCodecCtx, pFormatCtx->streams[videoStream]->time_base);

    // 打开解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
        return -1; // 打开失败

    // 申请视频帧数据
    pFrame = av_frame_alloc();

    // 复制帧数据
    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
        return -1;

    // 确定所需的缓冲区大小并分配缓冲区 AV_PIX_FMT_RGB24格式 源视频帧图像宽、高时所需的缓冲区大小
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));

    // 将缓冲区分配给pFrameRGB，用来存放图片数据
    avpicture_fill((AVPicture *)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);

    // 初始化SWS context缩放结构 用于控制缩放图像数据
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

    // 读取帧并将前五帧保存到磁盘
    i = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0) 
    {
        // packet是否为视频流
        if (packet.stream_index == videoStream) 
        {
            // 解码视频帧数据
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            // 是否获取到视频帧数据?
            if (frameFinished) 
            {
                // 将图片原始格式数据转换为RGB
                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
                    pFrame->linesize, 0, pCodecCtx->height,
                    pFrameRGB->data, pFrameRGB->linesize);

                // 保存帧数据到磁盘(测试只保存5帧)
                if (++i <= 5)
                {
                    saveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
                }
            }
        }

        // 释放av_read_frame读取出来的临时数据
        av_free_packet(&packet);
    }

    // 释放 RGB 内存
    av_free(buffer);
    av_frame_free(&pFrameRGB);

    // 释放 YUV 内存
    av_frame_free(&pFrame);

    // 关闭解码器
    avcodec_close(pCodecCtx);
    avcodec_close(pCodecCtxOrig);

    // 关闭视频文件
    avformat_close_input(&pFormatCtx);

    return 0;
}