/** @file   VideoDecoder.h
 *  @brief   ”∆µΩ‚¬Îdemo
 *  @note
 *  @author lesliefish
 *  @date   2019/06/06
 */
#pragma once

#include <string>

#ifdef _WIN32
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
}
#endif

using namespace std;

class VideoDecoder
{
public:
    VideoDecoder();
    ~VideoDecoder();

public:
    static void exec(const string& in, const string& out);

public:
    void doDecode(const string& in, const string& out);

private:
    void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt, const std::string& fileName);
    void saveYuv(unsigned char *buf, int wrap, int xsize, int ysize, const string& filename);
    void saveppm(AVFrame* frame, int width, int height, const string& fileName);
    int getVideoStreamId(const string& inputFile);

private:
    const AVCodec* m_avCodec{ nullptr };
    AVFormatContext* m_formatCtx{ nullptr };
    AVCodecParserContext* m_codecParserContext{ nullptr };
    AVCodecContext* m_codecContext{ nullptr };
    AVFrame* m_yuvFrame{ nullptr };  
    AVFrame* m_rgbFrame{ nullptr };
    AVPacket* m_avPacket{ nullptr };
    SwsContext* swsContext{ nullptr };
};