#include "PlayImages.h"
#include "ui_PlayImages.h"
#include <QPainter>
#include <QImage>
#include <future>
#include "module/VideoDecoder.h"

PlayImages::PlayImages(QWidget *parent)
    : QWidget(parent)
{
    ui = new Ui::PlayImages();
    ui->setupUi(this);
}

PlayImages::~PlayImages()
{
    delete ui;
}

void PlayImages::play()
{
    VideoDecoder decoder;
    connect(&decoder, &VideoDecoder::signalDecodeRgbImage, [&](const string& filePath)
    {
        //if (m_image == QImage())
        //{
        //    m_image = QImage(QString::fromStdString(filePath));
        //}
    });

    std::async([&]
    {
        // �����߳�ִ��
        decoder.doDecode("C:\\Users\\yulei10\\Videos\\aa.mp4", "C:\\Users\\yulei10\\Videos\\aa");
    });


}

void PlayImages::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height()); //�Ȼ��ɺ�ɫ

    if (m_image.size().width() <= 0) 
        return;

    // ��ͼ�񰴱������ųɺʹ���һ����С
    QImage img = m_image.scaled(this->size(), Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter.drawImage(QPoint(x, y), img); //����ͼ��
}
