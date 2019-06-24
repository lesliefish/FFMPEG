/** @file   FFmpegQtPlayer.h
 *  @brief  ����FFMPEG+Qt����Ƶ������
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
    // ���Žӿڴ�����Ƶ�ļ�·��
    void play(const std::string& filPath, int width = 1280, int height = 720);

private:
    // ��ʼ����������
    bool initDatas();
    // ��ȡ�� ��Ƶ�� ��Ƶ����
    bool getStreams();
    // �򿪽�����
    bool avcodecOpen();

protected:
    virtual void paintEvent(QPaintEvent *event);
    virtual void resizeEvent(QResizeEvent *event);
private:
    // �ļ�
    std::string m_filePath{};
    const AVCodec* m_avCodec{ nullptr };

    // �������������϶�Ľṹ��
    AVFormatContext* m_formatCtx{ nullptr };

    // ������
    AVCodecParserContext* m_codecParserContext{ nullptr };

    // �����ṹ�� ��������������͡������ʡ��������ȵ�
    AVCodecContext* m_codecContext{ nullptr };
    AVFrame* m_rgbFrame{ nullptr };// �����frame
    AVFrame* m_frame{ nullptr }; // �洢ת���frame
    AVPacket* m_avPacket{ nullptr };
    SwsContext* m_swsContext{ nullptr };

    int m_videoStream{ -1 }; // ��Ƶ��id

    // Ĭ�ϴ��ڴ�С
    int m_playerWidth{ 1280 };
    int m_playerHeight{ 720 };

    QImage m_image; // ���ڱ���ͼƬ
    std::thread m_thread; // ���߳�ִ�ж�ȡ��Ƶ����
};
