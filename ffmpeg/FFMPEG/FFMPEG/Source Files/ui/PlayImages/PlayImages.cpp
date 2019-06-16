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
        // 放在线程执行
        decoder.doDecode("C:\\Users\\yulei10\\Videos\\aa.mp4", "C:\\Users\\yulei10\\Videos\\aa");
    });


}

void PlayImages::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height()); //先画成黑色

    if (m_image.size().width() <= 0) 
        return;

    // 将图像按比例缩放成和窗口一样大小
    QImage img = m_image.scaled(this->size(), Qt::KeepAspectRatio);

    int x = this->width() - img.width();
    int y = this->height() - img.height();

    x /= 2;
    y /= 2;

    painter.drawImage(QPoint(x, y), img); //画出图像
}
