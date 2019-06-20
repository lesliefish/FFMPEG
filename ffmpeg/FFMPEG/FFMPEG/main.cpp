#include <QtWidgets/QApplication>
#include "module/FFmpegBasic.h"
#include "module/VideoDecoder.h"
#include "module/FFmpegLogger.h"
#include "players/SimpleSDLPlayer/SimpleSDLPlayer.h"
#include "players/FFmpegSDLPlayer/FFmpegSDLPlayer.h"
#include "sdl/SdlImageViewer.h"

using namespace sdl2;
int main(int argc, char *argv[])
{
    //QApplication a(argc, argv);

    //SimpleSDLPlayer simpleSDLPlayer(352, 288, 176, 144, "SDL2 Player");
    //simpleSDLPlayer.play("suzie_qcif.yuv");
    
    FFmpegSDLPlayer ffmpegSDLPlayer;
    ffmpegSDLPlayer.play("C:\\Users\\yulei10\\Downloads\\aa.mp4");

    SdlImageViewer viewer;
    viewer.displayImage("bmpfile.bmp");
    //return a.exec();

    return 0;
}
