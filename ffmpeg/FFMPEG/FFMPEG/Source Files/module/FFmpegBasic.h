/** @file   FFmpegBasic.h
 *  @brief  FFmpeg 基本信息
 *  @note
 *  @author lesliefish
 *  @date   2019/06/05
 */
#pragma once
#include <string>
#include <map>
using namespace std;

class FFmpegBasic
{

public:

    //* Protocol:  FFmpeg类库支持的协议
    static map<string, string> urlProtocolInfo();

    //* AVFormat:  FFmpeg类库支持的封装格式
    static map<string, string> avFormatInfo();

};