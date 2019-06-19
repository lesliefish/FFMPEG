#include <QtWidgets/QApplication>
#include "module/FFmpegBasic.h"
#include "module/VideoDecoder.h"
#include "module/FFmpegLogger.h"
#include "players/SimpleSDLPlayer/SimpleSDLPlayer.h"

int main(int argc, char *argv[])
{
    //QApplication a(argc, argv);

    SimpleSDLPlayer simpleSDLPlayer(352, 288, 176, 144, "SDL2 Player");
    simpleSDLPlayer.play("suzie_qcif.yuv");
    
    //return a.exec();

    return 0;
}
