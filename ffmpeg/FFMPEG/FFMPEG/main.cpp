#include <QtWidgets/QApplication>
#include "module/FFmpegBasic.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FFmpegBasic::urlProtocolInfo();
    FFmpegBasic::avFormatInfo();
    FFmpegBasic::avCoderInfo();

    return a.exec();
}
