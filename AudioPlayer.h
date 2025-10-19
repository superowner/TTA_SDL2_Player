#pragma once
#ifndef _AUDIOPLAYER_H_
#define _AUDIOPLAYER_H_
#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>

namespace Player
{
    #undef GetCurrentTime

    class AudioPlayer 
    {
    public:
        AudioPlayer(const std::string& filePath) { m_filePath = filePath; }
        virtual ~AudioPlayer() {}

        // 禁止拷贝构造和赋值
        AudioPlayer(const AudioPlayer&) = delete;
        AudioPlayer& operator=(const AudioPlayer&) = delete;

        // 初始化播放
        virtual bool InitializePlayback() = 0;

        // 播放控制
        virtual void Play() = 0;
        virtual void Pause() = 0;
        virtual void Stop() = 0;
        virtual bool IsPlaying() const = 0;
        virtual bool IsPaused() const = 0;

        // 音量控制
        virtual float GetVolume() const = 0;
        virtual void SetVolume(float volume) = 0;// 0.0到1.0之间

        // 快进/快退（秒）
        virtual void FastForward(float seconds) = 0;
        virtual void Rewind(float seconds) = 0;

        // 根据时间设置播放位置（秒）
        virtual void SetPositionByTime(float seconds) = 0;
        // 获取当前播放时间
        virtual float GetCurrentTime() const = 0;
        // 获取播放总时间
        virtual float GetTotalTime() const = 0;

        bool FileState() const { return file_exists(m_filePath); }
        std::string FileExtension() const { return get_file_extension(m_filePath); }
        std::string GetFile() const { return m_filePath; }

    private:
        std::string m_filePath;
        static bool file_exists(const std::string& filename) 
        {
            if (0 == filename.length())
                return false;

            std::ifstream file(filename);
            bool result = file.good();  // 如果文件成功打开，返回 true
            file.close();
            return result;
        }
        static std::string to_lower(const std::string &_str) 
        {
            std::string str{ _str };
            std::transform(str.begin(), str.end(), str.begin(),
                [](unsigned char c) { return std::tolower(c); });
            return str;
        }
        static std::string get_file_extension(const std::string& filename) 
        {
            if (0 == filename.length())
                return "";

            size_t dot_pos = filename.find_last_of('.');
            if (dot_pos != std::string::npos) {
                return to_lower(filename.substr(dot_pos+1));
            }
            return ""; // 没有扩展名
        }
    };
}
#endif // !_AUDIOPLAYER_H_
