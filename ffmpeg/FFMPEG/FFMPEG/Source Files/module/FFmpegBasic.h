/** @file   FFmpegBasic.h
 *  @brief  FFmpeg ������Ϣ
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

    //* Protocol:  FFmpeg���֧�ֵ�Э��
    static map<string, string> urlProtocolInfo();

    //* AVFormat:  FFmpeg���֧�ֵķ�װ��ʽ
    static map<string, string> avFormatInfo();

    //* AVCodec:   FFmpeg���֧�ֵı������
    static map<string, vector<pair<string, string>>> avCoderInfo();
};