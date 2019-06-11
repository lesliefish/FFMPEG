#pragma once

#include "stdio.h"
#include <iostream>
#include <string>

#ifdef _WIN32
extern "C"
{
#include "libavutil/log.h"
}
#endif

class FFmpegLogger
{
public:
    static void logCallback(void* ptr, int level, const char* format, va_list vl);

    static void init();

    static void write(int level, const std::string& str);
};