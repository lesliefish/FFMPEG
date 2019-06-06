#include <QtWidgets/QApplication>
#include "module/FFmpegBasic.h"
#include "module/VideoDecoder.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FFmpegBasic::urlProtocolInfo();
    FFmpegBasic::avFormatInfo();
    FFmpegBasic::avCoderInfo();
    FFmpegBasic::avFilterInfo();
    FFmpegBasic::configurationInfo();
     
    VideoDecoder::exec("C:/Users/yulei10/Downloads/ss.mov", "C:/Users/yulei10/Downloads/ss.jpeg");
    return a.exec();
}
