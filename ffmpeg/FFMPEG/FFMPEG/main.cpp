#include <QtWidgets/QApplication>
#include "module/FFmpegBasic.h"
#include "module/VideoDecoder.h"
#include "module/FFmpegLogger.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FFmpegLogger::init();
    FFmpegBasic::urlProtocolInfo();
    FFmpegBasic::avFormatInfo();
    FFmpegBasic::avCoderInfo();
    FFmpegBasic::avFilterInfo();
    FFmpegBasic::configurationInfo();
     
    VideoDecoder::exec("C:\\Users\\aa\\Videos\\aa.mp4", "C:\\Users\\aa\\Videos\\aa.yuv");
    return a.exec();
}
