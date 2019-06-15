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
    void pgmSave(unsigned char *buf, int wrap, int xsize, int ysize, const string& filename);
    int getVideoStreamId(const string& inputFile);

private:
    string m_videoFileName;
    string m_outFileName;
    const AVCodec* m_avCodec{ nullptr };
    AVFormatContext* m_formatCtx{ nullptr };
    AVCodecParserContext* m_codecParserContext{ nullptr };
    AVCodecContext* m_codecContext{ nullptr };
    AVFrame* m_frame{ nullptr };
    AVPacket* m_avPacket{ nullptr };
};