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
	// ǰ������
	typedef struct TTAIOWrapper TTAIOWrapper;

	class TtaAudioPlayer :public AudioPlayer 
	{
	public:
		TtaAudioPlayer(const std::string& filePath);
		virtual ~TtaAudioPlayer();

		// ��ֹ��������͸�ֵ
		TtaAudioPlayer(const TtaAudioPlayer&) = delete;
		TtaAudioPlayer& operator=(const TtaAudioPlayer&) = delete;

		// ��ʼ������
		virtual bool InitializePlayback() override;

		// ���ſ���
		virtual void Play() override;
		virtual void Pause() override;
		virtual void Stop() override;
		virtual bool IsPlaying() const override;
		virtual bool IsPaused() const override;

		// ��������
		virtual float GetVolume() const override;
		virtual void SetVolume(float volume) override; // 0.0��1.0֮��

		// ���/���ˣ��룩
		virtual void FastForward(float seconds) override;
		virtual void Rewind(float seconds) override;

		// ����ʱ�����ò���λ�ã��룩
		virtual void SetPositionByTime(float seconds) override;
		// ��ȡ��ǰ����ʱ��
		virtual float GetCurrentTime() const override;
		// ��ȡ������ʱ��
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
		
		void Mute();// ǿ�ƾ���
		void Unmute();// ȡ������
		bool IsMuted() const;// ��龲��״̬
		void Sequence();

		static constexpr int PCM_BUFFER_LENGTH = 4096;// ���뻺����������
		std::string m_filePath;
		void* m_handle;                               // TTA SDK���ļ����
		TTAIOWrapper* m_ioWrapper;                    // TTA IO�ص���װ��
		void* aligned_decoder;
		void* info;
		void* m_decoder;                              // TTA������ʵ��
		SDL_AudioSpec m_audioSpec;                    // SDL��Ƶ���
		SDL_AudioDeviceID m_audioDevice = 0;          // SDL��Ƶ�豸ID
		Uint8 m_pcmBuffer[PCM_BUFFER_LENGTH * 4 * 2]; // ���뻺���������֧��2ͨ��32λ��

		mutable std::mutex m_mutex;                   // ��������״̬�Ļ�����
		bool m_isPlaying;                             // ����״̬
		bool m_isStopped;                             // ����״̬
		bool m_isPaused;                              // ��ͣ״̬
		std::atomic<float> m_volume;                  // ������ԭ�ӱ���ȷ���̰߳�ȫ��
		uint32_t m_sampleRate;                        // ������
		uint16_t m_channels;                          // ������
		uint16_t m_bitsPerSample;                     // λ��
		uint32_t m_totalSamples;                      // ��������
		uint32_t m_currentSample;                     // ��ǰ��������λ��
		uint32_t m_seekSkip;                          // ��λ����Ҫ������������
		bool isMuted_ = false;
		float savedVolume_ = 1.0f;
		std::thread worker_thread;
		
	};
}
#endif // !_FOOBAR_HPP_
