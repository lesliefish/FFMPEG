#pragma once

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL/SDL.h"
}

#include <stdio.h>
#include <QObject>

class SimpleSDLPlayer : public QObject
{
    Q_OBJECT

public:
    SimpleSDLPlayer(QObject *parent = Q_NULLPTR);
    ~SimpleSDLPlayer();

public:
    void play(const std::string& filePath);
};