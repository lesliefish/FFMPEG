#pragma once

#include <QWidget>
namespace Ui { class PlayImages; };

class VideoDecoder;
class QImage;
class PlayImages : public QWidget
{
    Q_OBJECT

public:
    PlayImages(QWidget *parent = Q_NULLPTR);
    ~PlayImages();

    void play();

protected:
    virtual void paintEvent(QPaintEvent *event);

private:
    Ui::PlayImages *ui;

    QImage m_image{};
};
