/** @file   FFmpegBasic.h
 *  @brief  FFmpeg 基本信息
 *  @note
 *  @author lesliefish
 *  @date   2019/06/05
 */
#pragma once
#include <string>
#include <map>
#include <vector>

using namespace std;

class FFmpegBasic
{

public:

    //* Protocol:  FFmpeg类库支持的协议
    static map<string, string> urlProtocolInfo();

    //* AVFormat:  FFmpeg类库支持的封装格式
    static map<string, string> avFormatInfo();

    //* AVCodec:   FFmpeg类库支持的编解码器
    static map<string, vector<pair<string, string>>> avCoderInfo();
};