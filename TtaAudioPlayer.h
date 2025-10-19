#ifndef _FOOBAR_HPP_
#define _FOOBAR_HPP_

#include <string>
#include <SDL.h>
#include <atomic>
#include <mutex>
#include "AudioPlayer.h"

namespace Player 
{

	using namespace std;
	// 前向声明
	typedef struct TTAIOWrapper TTAIOWrapper;

	class TtaAudioPlayer :public AudioPlayer 
	{
	public:
		TtaAudioPlayer(const std::string& filePath);
		virtual ~TtaAudioPlayer();

		// 禁止拷贝构造和赋值
		TtaAudioPlayer(const TtaAudioPlayer&) = delete;
		TtaAudioPlayer& operator=(const TtaAudioPlayer&) = delete;

		// 初始化播放
		virtual bool InitializePlayback() override;

		// 播放控制
		virtual void Play() override;
		virtual void Pause() override;
		virtual void Stop() override;
		virtual bool IsPlaying() const override;
		virtual bool IsPaused() const override;

		// 音量控制
		virtual float GetVolume() const override;
		virtual void SetVolume(float volume) override; // 0.0到1.0之间

		// 快进/快退（秒）
		virtual void FastForward(float seconds) override;
		virtual void Rewind(float seconds) override;

		// 根据时间设置播放位置（秒）
		virtual void SetPositionByTime(float seconds) override;
		// 获取当前播放时间
		virtual float GetCurrentTime() const override;
		// 获取播放总时间
		virtual float GetTotalTime() const override;

	private:
		static void audioCallback(void* userdata, Uint8* stream, int len);
		void fillAudioBuffer(Uint8* stream, int len);
		void applyVolumeAndCopy(
			const Uint8* src,
			Uint8* dst,
			uint32_t samples,
			int sampleSize,
			int bitsPerSample,
			float volume
		);
		
		void Mute();// 强制静音
		void Unmute();// 取消静音
		bool IsMuted() const;// 检查静音状态
		void Sequence();

		static constexpr int PCM_BUFFER_LENGTH = 4096;// 解码缓冲区样本数
		std::string m_filePath;
		void* m_handle;                               // TTA SDK的文件句柄
		TTAIOWrapper* m_ioWrapper;                    // TTA IO回调包装器
		void* aligned_decoder;
		void* info;
		void* m_decoder;                              // TTA解码器实例
		SDL_AudioSpec m_audioSpec;                    // SDL音频规格
		SDL_AudioDeviceID m_audioDevice = 0;          // SDL音频设备ID
		Uint8 m_pcmBuffer[PCM_BUFFER_LENGTH * 4 * 2]; // 解码缓冲区（最大支持2通道32位）

		mutable std::mutex m_mutex;                   // 保护共享状态的互斥锁
		bool m_isPlaying;                             // 播放状态
		bool m_isStopped;                             // 播放状态
		bool m_isPaused;                              // 暂停状态
		std::atomic<float> m_volume;                  // 音量（原子变量确保线程安全）
		uint32_t m_sampleRate;                        // 采样率
		uint16_t m_channels;                          // 声道数
		uint16_t m_bitsPerSample;                     // 位深
		uint32_t m_totalSamples;                      // 总样本数
		uint32_t m_currentSample;                     // 当前播放样本位置
		uint32_t m_seekSkip;                          // 定位后需要跳过的样本数
		bool isMuted_ = false;
		float savedVolume_ = 1.0f;
		std::thread worker_thread;
		
	};
}
#endif // !_FOOBAR_HPP_
