/** @file   FFmpegBasic.h
 *  @brief  FFmpeg 基本信息
 *  @note
 *  @author lesliefish
 *  @date   2019/06/05
 */
#pragma once
#include <string>

using std::string;

class FFmpegBasic
{

public:

    //* Protocol:  FFmpeg类库支持的协议
    static string urlProtocolInfo(const int type);

};