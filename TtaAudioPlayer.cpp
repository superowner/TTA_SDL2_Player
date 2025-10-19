#include "TtaAudioPlayer.h"

#include <fstream>
#include <stdexcept>
#include <cmath>
#include <libtta.h>
#include <config.h>
#include "tta.h"
#include <algorithm>
#include <iostream>
#include <conio.h>
#include <cctype>
#include <map>
#include <vector>

#include <locale>
#include <codecvt>

#undef min
#undef max
#undef GetCurrentTime

namespace Player
{
	using namespace tta;
	using namespace std;

	//tta_decoder *_m_decoder;

	// TTA IO�ص���װ����ʹ��TTA SDK��HANDLE��
	struct TTAIOWrapper {
		TTA_io_callback io;
		HANDLE handle;
	};

	static void CALLBACK tta_callback(TTAuint32 rate, TTAuint32 fnum, TTAuint32 frames) {
		TTAuint32 pcnt = (TTAuint32)(fnum * 100. / frames);
		//if (!(pcnt % 10))
		//    tta_print("\rProgress: %02d%%", pcnt);
	} // tta_callback

	// TTA IO�ص�����������TTA SDK���ļ�������
	static TTAint32 CALLBACK read_callback(TTA_io_callback* io, TTAuint8* buffer, TTAuint32 size) {
		TTAIOWrapper* iocb = (TTAIOWrapper*)io;
		TTAint32 result;
		if (tta_read(iocb->handle, buffer, size, result))
			return result;
		return 0;
	} // read_callback

	static TTAint64 CALLBACK seek_callback(TTA_io_callback* io, TTAint64 offset) {
		TTAIOWrapper* iocb = (TTAIOWrapper*)io;
		return tta_seek(iocb->handle, offset);
	} // seek_callback

	static std::wstring utf8_to_wstring(const std::string& str) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.from_bytes(str);
	}


	TtaAudioPlayer::TtaAudioPlayer(const std::string& filePath)
		:AudioPlayer(filePath), m_filePath(filePath), m_handle(INVALID_HANDLE_VALUE), m_decoder(nullptr),
		m_isPlaying(false), m_isPaused(false), m_volume(1.0f),
		m_sampleRate(0), m_channels(0), m_bitsPerSample(0),
		m_totalSamples(0), m_currentSample(0), m_seekSkip(0),
		m_isStopped(false), aligned_decoder(nullptr), info(nullptr), m_ioWrapper(nullptr)
	{
		if (SDL_Init(SDL_INIT_AUDIO) < 0) {
			throw std::runtime_error("SDL��ʼ��ʧ��: " + std::string(SDL_GetError()));
		}
	}

	TtaAudioPlayer::~TtaAudioPlayer()
	{
		Stop();
		if (m_decoder) {
			delete m_decoder;
			m_decoder = nullptr;
		}
		if (m_handle != INVALID_HANDLE_VALUE) {
			tta_close(m_handle);  // ʹ��TTA SDK�ر��ļ�
			m_handle = INVALID_HANDLE_VALUE;
		}

		if (nullptr != aligned_decoder)
		{
			delete[] aligned_decoder;
			aligned_decoder = nullptr;
		}
		if (nullptr != info)
		{
			delete info;
			info = nullptr;
		}
		if (nullptr != m_ioWrapper)
		{
			delete m_ioWrapper;
			m_ioWrapper = nullptr;
		}

		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}

	bool TtaAudioPlayer::InitializePlayback()
	{
		// ʹ��TTA SDK���ļ���ֻ��ģʽ��
		m_handle = tta_open_read(utf8_to_wstring(m_filePath).c_str());

		if (m_handle == INVALID_HANDLE_VALUE) {
			return false;
		}

		// ��ʼ��TTA IO�ص�����TTA SDK���ļ�������
		m_ioWrapper = new TTAIOWrapper();
		//TTAIOWrapper m_ioWrapper;
		m_ioWrapper->handle = m_handle;
		m_ioWrapper->io.read = read_callback;
		m_ioWrapper->io.seek = seek_callback;
		m_ioWrapper->io.write = NULL;  // ֻ��ģʽ������Ҫд�ص�

		aligned_decoder = tta_malloc(sizeof(tta::tta_decoder));
		m_decoder = new(aligned_decoder)tta_decoder((TTA_io_callback*)&m_ioWrapper->io);

		info = new TTA_info();

		try {
			((tta_decoder*)m_decoder)->init_get_info((TTA_info*)info, 0);
		}
		catch (tta_exception ex) {
			//tta_strerror(ex.code());
			//goto done;
			std::cout << "read error x:" << ex.what() << std::endl;
		}

		/*       std::cout << "---flag1---" << std::endl;
			   system("pause");*/

			   // ��ʼ��������
		try
		{
			m_sampleRate = ((TTA_info*)info)->sps;
			m_channels = ((TTA_info*)info)->nch;
			m_bitsPerSample = ((TTA_info*)info)->bps;
			m_totalSamples = ((TTA_info*)info)->samples;

			std::cout << "SampleRate:" << m_sampleRate << std::endl;
			std::cout << "Channels:" << m_channels << std::endl;
			std::cout << "BitsPerSample:" << m_bitsPerSample << std::endl;
			std::cout << "TotalSamples:" << m_totalSamples << std::endl;

			// ��֤��Ƶ������Ч��
			if (m_sampleRate <= 0 || m_channels <= 0 || m_bitsPerSample <= 0) {
				throw std::runtime_error("��Ч����Ƶ����");
			}

			// ����SDL��Ƶ���
			m_audioSpec.freq = m_sampleRate;
			// ����λ������SDL��ʽ��TTAͨ��Ϊ16/24/32λ��
			if (m_bitsPerSample == 16) {
				m_audioSpec.format = AUDIO_S16LSB;  // 16λ�з�������
			}
			else if (m_bitsPerSample == 24) {
				m_audioSpec.format = AUDIO_S32LSB;  // SDL��ԭ��24λ����32λ����
			}
			else if (m_bitsPerSample == 32) {
				m_audioSpec.format = AUDIO_S32LSB;  // 32λ�з�������
			}
			else {
				throw std::runtime_error("��֧�ֵ�λ��: " + std::to_string(m_bitsPerSample));
			}
			m_audioSpec.channels = m_channels;
			m_audioSpec.samples = 4096;  // ��Ƶ��������С����������
			m_audioSpec.callback = audioCallback;
			m_audioSpec.userdata = this;

			// ��SDL��Ƶ�豸
			m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &m_audioSpec, nullptr, 0);
			if (m_audioDevice == 0) {
				throw std::runtime_error("����Ƶ�豸ʧ��: " + std::string(SDL_GetError()));
			}

			return true;
		}
		catch (const std::exception& e)
		{
			std::cout << "read error:" << e.what() << std::endl;
			// ��ʼ��ʧ��ʱ������Դ
			if (m_decoder) {
				delete m_decoder;
				m_decoder = nullptr;
			}
			if (m_handle != INVALID_HANDLE_VALUE) {
				tta_close(m_handle);
				m_handle = INVALID_HANDLE_VALUE;
			}
			return false;
		}

		m_isStopped = false;
	}

	void TtaAudioPlayer::Mute()
	{
		if (!isMuted_) {
			savedVolume_ = m_volume.load(std::memory_order_acquire);
			m_volume.store(0, std::memory_order_release);
			isMuted_ = true;
		}
	}

	void TtaAudioPlayer::Unmute()
	{
		if (isMuted_) {
			m_volume.store(savedVolume_, std::memory_order_release);
			isMuted_ = false;
		}
	}

	bool TtaAudioPlayer::IsMuted() const
	{
		return isMuted_;
	}

	void TtaAudioPlayer::Sequence()
	{
		Mute();
		std::this_thread::sleep_for(std::chrono::seconds(3));
		// �����Ƶ�豸����
		SDL_ClearQueuedAudio(m_audioDevice);
		Unmute();
		m_isStopped = false;
	}
	void TtaAudioPlayer::Play()
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		// ����ǰ������ڲ�PCM������
		tta_memclear(m_pcmBuffer, sizeof(m_pcmBuffer));

		if (!m_isPlaying) {
			m_isPlaying = true;
			m_isPaused = false;
			// ��������ǰ��ȷ���豸���ڸɾ�״̬
			SDL_PauseAudioDevice(m_audioDevice, 0);
		}
		else if (m_isPaused) {
			m_isPaused = false;
			SDL_PauseAudioDevice(m_audioDevice, 0);
		}

		if (m_isStopped) {
			// �����߳�״̬
			if (worker_thread.joinable()) {
				worker_thread.join();
			}
			worker_thread = std::thread(&TtaAudioPlayer::Sequence, this);
			m_isStopped = false;
		}
	}
	void TtaAudioPlayer::Pause()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_isPlaying && !m_isPaused) {
			m_isPaused = true;
			SDL_PauseAudioDevice(m_audioDevice, 1);  // ��ͣ����
		}
	}
	void TtaAudioPlayer::Stop()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_isPlaying)
		{
			m_isPlaying = false;
			m_isPaused = false;
			SDL_PauseAudioDevice(m_audioDevice, 1);  // ��ͣ����

			// ���SDL��Ƶ�������������������
			SDL_ClearQueuedAudio(m_audioDevice);

			// ���ò���λ�úͽ�����״̬
			m_currentSample = 0;
			m_seekSkip = 0;

			if (m_decoder) {
				((tta_decoder*)m_decoder)->frame_reset(0, &m_ioWrapper->io);
			}
			//if (m_handle != INVALID_HANDLE_VALUE) {
			//    tta_seek((HANDLE)m_handle, (int64_t)0);  // ȷ���ļ�ָ������
			//}
			if (m_handle != INVALID_HANDLE_VALUE) {
				//TTAint64 result = tta_seek(m_handle, 0);  // ���շ���ֵ
				int64_t offset = 0;
				TTAint64 result = SetFilePointer(m_handle, (LONG)offset, (PLONG)&offset + 1, FILE_BEGIN);
				if (result != 0) {  // ����0��ʾ�ɹ���������ݿⶨ�����
					// ��ѡ������seekʧ�ܵ����
					// std::cerr << "Failed to seek to start: " << result << std::endl;
				}
			}

			m_isStopped = true;
		}
	}
	bool TtaAudioPlayer::IsPlaying() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_isPlaying && !m_isPaused;
	}
	bool TtaAudioPlayer::IsPaused() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_isPaused;
	}
	float TtaAudioPlayer::GetVolume() const
	{
		return m_volume.load(std::memory_order_acquire);
	}
	void TtaAudioPlayer::SetVolume(float volume)
	{
		// ����������Χ��0.0~1.0
		volume = std::clamp(volume, 0.0f, 1.0f);
		m_volume.store(volume, std::memory_order_release);
	}
	void TtaAudioPlayer::FastForward(float seconds)
	{
		SetPositionByTime(GetCurrentTime() + seconds);
	}
	void TtaAudioPlayer::Rewind(float seconds)
	{
		SetPositionByTime(GetCurrentTime() - seconds);
	}
	void TtaAudioPlayer::SetPositionByTime(float seconds)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (seconds < 0 || seconds > GetTotalTime()) {
			return;
		}

		// ��ȫֹͣ��Ƶ��
		SDL_PauseAudioDevice(m_audioDevice, 1);

		// ���ý�����״̬
		if (m_decoder) {
			((tta_decoder*)m_decoder)->frame_reset(0, &m_ioWrapper->io);
		}

		// ����Ŀ������λ��
		m_currentSample = static_cast<uint32_t>(seconds * m_sampleRate);

		// ͨ����������λ��Ŀ��ʱ���
		uint32_t frameStart;
		((tta_decoder*)m_decoder)->set_position(seconds, &frameStart);

		// ������Ҫ������������
		m_seekSkip = static_cast<uint32_t>((seconds - frameStart) * m_sampleRate + 0.5);
		m_seekSkip = std::min(m_seekSkip, ((tta_decoder*)m_decoder)->get_flen());

		// ���벢����seekSkip������������ȷ��������״̬��ȷ
		if (m_seekSkip > 0) {
			const int sampleSize = m_channels * (m_bitsPerSample / 8);
			uint32_t samplesToDiscard = m_seekSkip;
			while (samplesToDiscard > 0) {
				uint32_t decodeSize = std::min(samplesToDiscard, (uint32_t)PCM_BUFFER_LENGTH);
				uint32_t decoded = ((tta_decoder*)m_decoder)->process_stream(m_pcmBuffer, decodeSize, tta_callback);
				if (decoded == 0) break;
				samplesToDiscard -= decoded;
			}
			m_seekSkip = 0;
		}

		// �����Ƶ�豸����
		SDL_ClearQueuedAudio(m_audioDevice);

		// ���¿�ʼ����
		if (m_isPlaying && !m_isPaused) {
			SDL_PauseAudioDevice(m_audioDevice, 0);
		}
	}
	float TtaAudioPlayer::GetCurrentTime() const
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return static_cast<float>(m_currentSample) / m_sampleRate;
	}
	float TtaAudioPlayer::GetTotalTime() const
	{
		return static_cast<float>(m_totalSamples) / m_sampleRate;
	}


	//privative
	void TtaAudioPlayer::audioCallback(void* userdata, Uint8* stream, int len)
	{
		TtaAudioPlayer* player = reinterpret_cast<TtaAudioPlayer*>(userdata);
		player->fillAudioBuffer(stream, len);
	}

	void TtaAudioPlayer::fillAudioBuffer(Uint8* stream, int len)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_isPlaying || m_isPaused || !m_decoder) {
			tta_memclear(stream, len);
			return;
		}

		// �״β��Ż�մ�ֹͣ״̬�ָ�ʱ�������һС�ξ���
		static bool firstFrame = true;
		if (firstFrame && m_currentSample == 0) {
			int silenceLen = std::min(len, 1024);  // 1024�ֽھ���
			tta_memclear(stream, silenceLen);
			stream += silenceLen;
			len -= silenceLen;
			firstFrame = false;
		}
		else {
			firstFrame = false;
		}

		// ʣ��ԭ���߼�...
		Uint8* bufferPtr = stream;
		int remaining = len;
		const int sampleSize = m_channels * (m_bitsPerSample / 8);
		const float volume = m_volume.load(std::memory_order_acquire);

		while (remaining > 0)
		{
			// ����seekʱ��Ҫ����������
			if (m_seekSkip > 0) {
				uint32_t skipBytes = std::min(m_seekSkip * sampleSize, static_cast<uint32_t>(remaining));
				m_seekSkip -= skipBytes / sampleSize;
				bufferPtr += skipBytes;
				remaining -= skipBytes;
				continue;
			}

			// ����TTA���ݣ�ʹ�ý�������ȡһ֡��
			//uint32_t decodedSamples = m_decoder->decode_stream(m_pcmBuffer, PCM_BUFFER_LENGTH);
			uint32_t decodedSamples = ((tta_decoder*)m_decoder)->process_stream(m_pcmBuffer, PCM_BUFFER_LENGTH, tta_callback);
			if (decodedSamples == 0) {
				// ������ɣ���侲��
				tta_memclear(bufferPtr, remaining);
				Stop();
				break;
			}

			// ����ɸ��Ƶ���������������ʣ�໺������С��
			uint32_t maxCopySamples = remaining / sampleSize;
			uint32_t copySamples = std::min(decodedSamples, maxCopySamples);
			uint32_t copyBytes = copySamples * sampleSize;

			// Ӧ�����������Ƶ�SDL���������
			applyVolumeAndCopy(
				m_pcmBuffer,
				bufferPtr,
				copySamples,
				sampleSize,
				m_bitsPerSample,
				volume
			);

			// ����ָ��ͼ�����
			bufferPtr += copyBytes;
			remaining -= copyBytes;
			m_currentSample += copySamples;
		}

	}//fillAudioBuffer

	void TtaAudioPlayer::applyVolumeAndCopy(const Uint8* src, Uint8* dst, uint32_t samples, int sampleSize, int bitsPerSample, float volume)
	{
		if (volume == 1.0f) {
			// ����Ϊ1ʱֱ�Ӹ��ƣ�ʹ��TTA SDK���ڴ渴�ƺ�����
			tta_memcpy(dst, src, samples * sampleSize);
			return;
		}

		// ����λ�������
		switch (bitsPerSample)
		{
		case 16: {
			const int16_t* src16 = reinterpret_cast<const int16_t*>(src);
			int16_t* dst16 = reinterpret_cast<int16_t*>(dst);
			for (uint32_t i = 0; i < samples * m_channels; ++i) {
				// ���Ų����Ʒ�Χ����ֹ�����
				dst16[i] = static_cast<int16_t>(std::clamp(
					static_cast<float>(src16[i]) * volume,
					static_cast<float>(INT16_MIN),
					static_cast<float>(INT16_MAX)
				));
			}
			break;
		}
		case 24: {
			// 24λ����ͨ��32λ������������24λ��Ч��
			const int32_t* src32 = reinterpret_cast<const int32_t*>(src);
			int32_t* dst32 = reinterpret_cast<int32_t*>(dst);
			for (uint32_t i = 0; i < samples * m_channels; ++i) {
				// ������չ�����ţ��ٽضϻ�24λ
				int32_t val = (src32[i] << 8) >> 8;  // ���ַ���λ
				val = static_cast<int32_t>(static_cast<float>(val) * volume);
				dst32[i] = (val << 8) >> 8;  // �ضϵ�24λ
			}
			break;
		}
		case 32: {
			const int32_t* src32 = reinterpret_cast<const int32_t*>(src);
			int32_t* dst32 = reinterpret_cast<int32_t*>(dst);
			for (uint32_t i = 0; i < samples * m_channels; ++i) {
				dst32[i] = static_cast<int32_t>(std::clamp(
					static_cast<float>(src32[i]) * volume,
					static_cast<float>(INT32_MIN),
					static_cast<float>(INT32_MAX)
				));
			}
			break;
		}
		default:
			// ��֧�ֵ�λ�ֱ�Ӹ���
			tta_memcpy(dst, src, samples * sampleSize);
		}
	}//applyVolumeAndCopy

}
