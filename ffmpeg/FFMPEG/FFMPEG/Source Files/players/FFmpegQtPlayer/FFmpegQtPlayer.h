/** @file   FFmpegQtPlayer.h
 *  @brief  基于FFMPEG+Qt的视频播放类
 *  @note
 *  @author lesliefish
 *  @date   2019/06/24
 */
#pragma once

#include <QWidget>
#include <QImage>
#include <string>
#include <thread>
#include <stdio.h>

#ifdef _WIN32
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/pixfmt.h"
#include "libavutil/imgutils.h"
}
#endif

class FFmpegQtPlayer : public QWidget
{
    Q_OBJECT

public:
    FFmpegQtPlayer(QWidget *parent = Q_NULLPTR);
    ~FFmpegQtPlayer();

signals:
    void signalRefresh();
public:
    // 播放接口传入视频文件路径
    void play(const std::string& filPath, int width = 1280, int height = 720);

private:
    // 初始化基本数据
    bool initDatas();
    // 获取流 音频流 视频流等
    bool getStreams();
    // 打开解码器
    bool avcodecOpen();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
private:
    // 文件
    std::string m_filePath{};
    const AVCodec* m_avCodec{ nullptr };

    // 包含码流参数较多的结构体
    AVFormatContext* m_formatCtx{ nullptr };

    // 解析器
    AVCodecParserContext* m_codecParserContext{ nullptr };

    // 变量结构体 包括编解码器类型、采样率、声道数等等
    AVCodecContext* m_codecContext{ nullptr };
    AVFrame* m_rgbFrame{ nullptr };// 解码后frame
    AVFrame* m_frame{ nullptr }; // 存储转码后frame
    AVPacket* m_avPacket{ nullptr };
    SwsContext* m_swsContext{ nullptr };

    int m_videoStream{ -1 }; // 视频流id

    // 默认窗口大小
    int m_playerWidth{ 1280 };
    int m_playerHeight{ 720 };

    QImage m_image; // 窗口背景图片
    std::thread m_thread; // 子线程执行读取视频数据
};
