/** @file   VideoDecoder.h
 *  @brief  视频解码demo
 *  @note
 *  @author lesliefish
 *  @date   2019/06/06
 */
#pragma once

#include <string>
#include <vector>
#include <QObject>

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

class VideoDecoder : public QObject
{
    Q_OBJECT
    
public:
    VideoDecoder();
    ~VideoDecoder();

signals:
    void signalDecodeEvent(const AVFrame& frame);

public:
    static void exec(const string& in, const string& out);

public:
    // 执行解码
    void doDecode(const string& in, const string& out);

    // 获取解码的rgb文件路径
    vector<string> getRgbFileList() { return m_rgbFilePathList; };

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
    SwsContext* m_swsContext{ nullptr };

    vector<string> m_rgbFilePathList{};
};