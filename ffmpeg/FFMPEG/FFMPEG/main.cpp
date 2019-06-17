#include <QtWidgets/QApplication>
#include "module/FFmpegBasic.h"
#include "module/VideoDecoder.h"
#include "module/FFmpegLogger.h"
#include "players/SimpleSDLPlayer/SimpleSDLPlayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    FFmpegLogger::init();    

    //FFmpegBasic::version();
    //FFmpegBasic::configurationInfo();
    //FFmpegBasic::urlProtocolInfo();
    //FFmpegBasic::avFormatInfo();
    //FFmpegBasic::avCoderInfo();
    //FFmpegBasic::avFilterInfo();

    //VideoDecoder::exec("C:\\Users\\yulei10\\Videos\\aa.mp4", "C:\\Users\\yulei10\\Videos\\aa");
    //playImages.play();
    SimpleSDLPlayer simpleSDLPlayer;
    simpleSDLPlayer.play("C:\\Users\\yulei10\\Videos\\aa.mp4");

    return a.exec();
}
