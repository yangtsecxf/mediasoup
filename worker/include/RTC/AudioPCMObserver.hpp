#ifndef MS_RTC_AUDIO_PCM_OBSERVER_HPP
#define MS_RTC_AUDIO_PCM_OBSERVER_HPP

#include "RTC/RtpObserver.hpp"
#include <nlohmann/json.hpp>
#include <unordered_map>
#include "opus.h"

using json = nlohmann::json;

namespace RTC
{
	class AudioPCMObserver : public RTC::RtpObserver
	{

	public:
		AudioPCMObserver(const std::string& id, json& data);
		~AudioPCMObserver() override;

	public:
		void AddProducer(RTC::Producer* producer) override;
		void RemoveProducer(RTC::Producer* producer) override;
		void ReceiveRtpPacket(RTC::Producer* producer, RTC::RtpPacket* packet) override;
		void ProducerPaused(RTC::Producer* producer) override;
		void ProducerResumed(RTC::Producer* producer) override;

    private: 
        void Paused() override;
		void Resumed() override;

        void OpenOpusDecoder();
        void CloseOpusDecoder();
        
	private:
		OpusDecoder *opusDecoder;
	};
} // namespace RTC

#endif
