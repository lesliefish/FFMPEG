#include "FFmpegLogger.h"

using namespace std;

void FFmpegLogger::logCallback(void* ptr, int level, const char* format, va_list vl)
{
    //if (level <= AV_LOG_WARNING)
    {
        cout << format << endl;
    }
}

void FFmpegLogger::init()
{
    av_log_set_callback(logCallback);
}

void FFmpegLogger::write(int level, const std::string& str)
{
    av_log(nullptr, level, str.c_str());
}
