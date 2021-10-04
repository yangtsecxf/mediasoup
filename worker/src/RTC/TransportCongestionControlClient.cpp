#define MS_CLASS "RTC::TransportCongestionControlClient"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/TransportCongestionControlClient.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <libwebrtc/api/transport/network_types.h> // webrtc::TargetRateConstraints
#include <limits>
#include <algorithm>
#include <numeric>
#include "log.h"
#include "../../statistics/Statistics.h"
#include "util.h"
using namespace std;

namespace RTC
{
	/* Static. */

	static constexpr uint32_t MinBitrate{ 30000u };
	static constexpr float MaxBitrateIncrementFactor{ 1.35f };
	static constexpr float MaxPaddingBitrateFactor{ 0.85f };
	static constexpr uint64_t AvailableBitrateEventInterval{ 2000u }; // In ms.

	/* Instance methods. */

	TransportCongestionControlClient::TransportCongestionControlClient(
	  RTC::TransportCongestionControlClient::Listener* listener,
	  RTC::BweType bweType,
	  uint32_t initialAvailableBitrate, 
		const std::string& transport_id)
	  : listener(listener), bweType(bweType),
	    initialAvailableBitrate(std::max<uint32_t>(initialAvailableBitrate, MinBitrate))
		, transport_id_(transport_id)
	{
		MS_TRACE();

		webrtc::GoogCcFactoryConfig config;

		// Provide RTCP feedback as well as Receiver Reports.
		config.feedback_only = false;

		this->controllerFactory = new webrtc::GoogCcNetworkControllerFactory(std::move(config));

		webrtc::BitrateConstraints bitrateConfig;

		bitrateConfig.start_bitrate_bps = static_cast<int>(this->initialAvailableBitrate);

		this->rtpTransportControllerSend =
		  new webrtc::RtpTransportControllerSend(this, nullptr, this->controllerFactory, bitrateConfig);

		this->rtpTransportControllerSend->RegisterTargetTransferRateObserver(this);

		this->probationGenerator = new RTC::RtpProbationGenerator();

		this->processTimer = new Timer(this);

		// NOTE: This is supposed to recover computed available bandwidth after
		// network issues.
		//this->rtpTransportControllerSend->EnablePeriodicAlrProbing(true);

		// clang-format off
		//INFO("[timer] TimeUntilNextProcess:", this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(), "GetProcessInterval:", this->controllerFactory->GetProcessInterval().ms());

		this->processTimer->Start(std::min(
			// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
			this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
			// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
			this->controllerFactory->GetProcessInterval().ms()
		));
		// clang-format on

#if 0
		string deltas_str = util::read_file("D://qz//webrtc//dump//time_deltas.txt");
		vector<string> deltas = util::s_split(deltas_str, ",");
		deltas_.reserve(deltas.size());
		for (size_t i = 0; i < deltas.size(); i++)
		{
			int16_t delta = atoi(deltas[i].c_str());
			if (delta > 100)
			{
				continue;
			}
			deltas_.push_back(delta);
		}
#endif // 0
	}

	TransportCongestionControlClient::~TransportCongestionControlClient()
	{
		MS_TRACE();

		delete this->controllerFactory;
		this->controllerFactory = nullptr;

		delete this->rtpTransportControllerSend;
		this->rtpTransportControllerSend = nullptr;

		delete this->probationGenerator;
		this->probationGenerator = nullptr;

		delete this->processTimer;
		this->processTimer = nullptr;
	}

	void TransportCongestionControlClient::TransportConnected()
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnNetworkAvailability(true);
	}

	void TransportCongestionControlClient::TransportDisconnected()
	{
		MS_TRACE();

		auto nowMs = DepLibUV::GetTimeMsInt64();

		this->bitrates.desiredBitrate          = 0u;
		this->bitrates.effectiveDesiredBitrate = 0u;

		this->desiredBitrateTrend.ForceUpdate(0u, nowMs);
		this->rtpTransportControllerSend->OnNetworkAvailability(false);
	}

	void TransportCongestionControlClient::InsertPacket(webrtc::RtpPacketSendInfo& packetInfo)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->packet_sender()->InsertPacket(packetInfo.length);
		this->rtpTransportControllerSend->OnAddPacket(packetInfo);
	}

	webrtc::PacedPacketInfo TransportCongestionControlClient::GetPacingInfo()
	{
		MS_TRACE();

		return this->rtpTransportControllerSend->packet_sender()->GetPacingInfo();
	}

	void TransportCongestionControlClient::PacketSent(webrtc::RtpPacketSendInfo& packetInfo, int64_t nowMs)
	{
		MS_TRACE();

		// Notify the transport feedback adapter about the sent packet.
		rtc::SentPacket sentPacket(packetInfo.transport_sequence_number, nowMs);
		this->rtpTransportControllerSend->OnSentPacket(sentPacket, packetInfo.length);
	}

	void TransportCongestionControlClient::ReceiveEstimatedBitrate(uint32_t bitrate)
	{
		MS_TRACE();

		this->rtpTransportControllerSend->OnReceivedEstimatedBitrate(bitrate);
	}

	void TransportCongestionControlClient::ReceiveRtcpReceiverReport(
	  RTC::RTCP::ReceiverReportPacket* packet, float rtt, int64_t nowMs)
	{
		MS_TRACE();

		webrtc::ReportBlockList reportBlockList;

		for (auto it = packet->Begin(); it != packet->End(); ++it)
		{
			auto& report = *it;

			reportBlockList.emplace_back(
			  packet->GetSsrc(),
			  report->GetSsrc(),
			  report->GetFractionLost(),
			  report->GetTotalLost(),
			  report->GetLastSeq(),
			  report->GetJitter(),
			  report->GetLastSenderReport(),
			  report->GetDelaySinceLastSenderReport());
		}

		this->rtpTransportControllerSend->OnReceivedRtcpReceiverReport(
		  reportBlockList, static_cast<int64_t>(rtt), nowMs);
	}

	void TransportCongestionControlClient::ReceiveRtcpTransportFeedback(RTC::RTCP::FeedbackRtpTransportPacket* feedback)
	{
		MS_TRACE();

		std::vector<int16_t>* p_deltas = feedback->get_deltas();
		for (int i=0; i< p_deltas->size(); ++i)
		{
			STS->update_time_delta((*p_deltas)[i], transport_id_);
			//if (deltas_index_ < deltas_.size())
			if (deltas_index_ <10000)
			{
				//(*p_deltas)[i] = deltas_[deltas_index_++];
				deltas_index_++;
			}
			else
			{
				STS->dump_all(transport_id_);
				deltas_index_ = 0;
			}
		}

		this->rtpTransportControllerSend->OnTransportFeedback(*feedback);
	}

	void TransportCongestionControlClient::SetDesiredBitrate(uint32_t desiredBitrate, bool force)
	{
		MS_TRACE();

		auto nowMs = DepLibUV::GetTimeMsInt64();

		// Manage it via trending and increase it a bit to avoid immediate oscillations.
		if (!force)
			this->desiredBitrateTrend.Update(desiredBitrate, nowMs);
		else
			this->desiredBitrateTrend.ForceUpdate(desiredBitrate, nowMs);

		this->bitrates.desiredBitrate          = desiredBitrate;
		this->bitrates.effectiveDesiredBitrate = this->desiredBitrateTrend.GetValue();
		this->bitrates.minBitrate              = MinBitrate;
		// NOTE: Setting 'startBitrate' to 'availableBitrate' has proven to generate
		// more stable values.
		this->bitrates.startBitrate = std::max<uint32_t>(MinBitrate, this->bitrates.availableBitrate);

		if (this->desiredBitrateTrend.GetValue() > 0u)
		{
			this->bitrates.maxBitrate = std::max<uint32_t>(
			  this->initialAvailableBitrate,
			  this->desiredBitrateTrend.GetValue() * MaxBitrateIncrementFactor);
			this->bitrates.maxPaddingBitrate = this->bitrates.maxBitrate * MaxPaddingBitrateFactor;
		}
		else
		{
			this->bitrates.maxBitrate        = this->initialAvailableBitrate;
			this->bitrates.maxPaddingBitrate = 0u;
		}

		MS_DEBUG_DEV(
		  "[desiredBitrate:%" PRIu32 ", startBitrate:%" PRIu32 ", minBitrate:%" PRIu32
		  ", maxBitrate:%" PRIu32 ", maxPaddingBitrate:%" PRIu32 "]",
		  this->bitrates.desiredBitrate,
		  this->bitrates.startBitrate,
		  this->bitrates.minBitrate,
		  this->bitrates.maxBitrate,
		  this->bitrates.maxPaddingBitrate);

		this->rtpTransportControllerSend->SetAllocatedSendBitrateLimits(
		  this->bitrates.minBitrate, this->bitrates.maxPaddingBitrate, this->bitrates.maxBitrate);

		webrtc::TargetRateConstraints constraints;

		constraints.at_time       = webrtc::Timestamp::ms(DepLibUV::GetTimeMs());
		constraints.min_data_rate = webrtc::DataRate::bps(this->bitrates.minBitrate);
		constraints.max_data_rate = webrtc::DataRate::bps(this->bitrates.maxBitrate);
		constraints.starting_rate = webrtc::DataRate::bps(this->bitrates.startBitrate);

		this->rtpTransportControllerSend->SetClientBitratePreferences(constraints);
	}

	uint32_t TransportCongestionControlClient::GetAvailableBitrate() const
	{
		MS_TRACE();

		return this->bitrates.availableBitrate;
	}

	void TransportCongestionControlClient::RescheduleNextAvailableBitrateEvent()
	{
		MS_TRACE();

		this->lastAvailableBitrateEventAtMs = DepLibUV::GetTimeMs();
	}

	void TransportCongestionControlClient::MayEmitAvailableBitrateEvent(uint32_t previousAvailableBitrate)
	{
		MS_TRACE();

		uint64_t nowMs = DepLibUV::GetTimeMsInt64();
		bool notify{ false };

		// Ignore if first event.
		// NOTE: Otherwise it will make the Transport crash since this event also happens
		// during the constructor of this class.
		// ����ǵ�һ���¼�������ԡ�
		// ע�⣺����ᵼ�´����������Ϊ����¼�Ҳ�ᷢ��
		// �������Ĺ��캯���С�
		if (this->lastAvailableBitrateEventAtMs == 0u)
		{
			this->lastAvailableBitrateEventAtMs = nowMs;

			return;
		}

		// Emit if this is the first valid event.
		// ������ǵ�һ����Ч�¼����򷢳���
		if (!this->availableBitrateEventCalled)
		{
			this->availableBitrateEventCalled = true;

			notify = true;
			//MS_DEBUG_TAG(bwe, "[cst]Emit if this is the first valid event.");
		}
		// Emit event if AvailableBitrateEventInterval elapsed.
		// ��� AvailableBitrateEventInterval �ѹ����򷢳��¼���
		else if (nowMs - this->lastAvailableBitrateEventAtMs >= AvailableBitrateEventInterval)
		{
			notify = true;
			//MS_DEBUG_TAG(bwe, "[cst]Emit event if AvailableBitrateEventInterval elapsed.");
		}
		// Also emit the event fast if we detect a high BWE value decrease.
		// ������Ǽ�⵽�� BWE ֵ�½���Ҳ���Կ��ٷ����¼���
		else if (this->bitrates.availableBitrate < previousAvailableBitrate * 0.75)
		{
			MS_WARN_TAG(
			  bwe,
			  "high BWE value decrease detected, notifying the listener [now:%" PRIu32 ", before:%" PRIu32
			  "]",
			  this->bitrates.availableBitrate,
			  previousAvailableBitrate);

			notify = true;
			//MS_DEBUG_TAG(bwe, "[cst]Also emit the event fast if we detect a high BWE value decrease.");
		}
		// Also emit the event fast if we detect a high BWE value increase.
		// ������Ǽ�⵽�� BWE ֵ���ӣ�Ҳ����ٷ����¼���
		else if (this->bitrates.availableBitrate > previousAvailableBitrate * 1.50)
		{
			MS_DEBUG_TAG(
			  bwe,
			  "high BWE value increase detected, notifying the listener [now:%" PRIu32 ", before:%" PRIu32
			  "]",
			  this->bitrates.availableBitrate,
			  previousAvailableBitrate);

			notify = true;
			//MS_DEBUG_TAG(bwe, "[cst]Also emit the event fast if we detect a high BWE value increase.");
		}

		//nosie alghrithm////////////////////////////////////////////////////////////////////////
		size_t max_size = 10;
		uint32_t availableBitrate = this->bitrates.availableBitrate;

		// the bitrate is the max, should not notify.
		int mean = vec_.size() > 0 
			? std::accumulate(std::begin(vec_), std::end(vec_), 0.0) / vec_.size() 
			: 0;
		if (vec_.size() >= max_size
			&& availableBitrate >= mean * 2.8
			)
		{
			//notify = false;
			//MS_WARN_TAG(bwe, "[cst] the bitrate is the max, should not notify. max: %u(kb) average: %u(kb) vector_size: %d", 
			//	availableBitrate/(1024*8), mean/(1024*8), vec_.size());
			//return;
		}

		vec_.push_back(availableBitrate);

		// keep the set with 5 elements
		if (vec_.size() > max_size)
		{
			vec_.erase(vec_.begin());
		}
		//////////////////////////////////////////////////////////////////////////

		if (notify)
		{
			MS_DEBUG_DEV(
			  "notifying the listener with new available bitrate:%" PRIu32,
			  availableBitrate);

			this->lastAvailableBitrateEventAtMs = nowMs;
			
 			if (bitrate_kb_ > 0)
 			{
 				bitrate_kb_ = bitrate_kb_;
 				this->bitrates.availableBitrate = bitrate_kb_ * 1024 * 8;
 			}
			
			this->listener->OnTransportCongestionControlClientBitrates(this, this->bitrates);
		}
	}

	void TransportCongestionControlClient::OnTargetTransferRate(webrtc::TargetTransferRate targetTransferRate)
	{
		MS_TRACE();

		// NOTE: The same value as 'this->initialAvailableBitrate' is received periodically
		// regardless of the real available bitrate. Skip such value except for the first time
		// this event is called.
		// clang-format off
		// ע�⣺���ڽ����� 'this->initialAvailableBitrate' ��ͬ��ֵ
		// ����ʵ�ʿ��õı����ʡ�����һ����������ֵ
		// ����¼������á�
		// clang ��ʽ�ر�
		if (
			this->availableBitrateEventCalled &&
			targetTransferRate.target_rate.bps() == this->initialAvailableBitrate
		)
		// clang-format on
		{
			return;
		}

		auto previousAvailableBitrate = this->bitrates.availableBitrate;

		// Update availableBitrate.
		// NOTE: Just in case.
		// target_rate���������max������
		if (targetTransferRate.target_rate.bps() > std::numeric_limits<uint32_t>::max())
			this->bitrates.availableBitrate = std::numeric_limits<uint32_t>::max();
		else		// availableBitrate = target_rate
			this->bitrates.availableBitrate = static_cast<uint32_t>(targetTransferRate.target_rate.bps());

		MS_DEBUG_DEV("new available bitrate:%" PRIu32, this->bitrates.availableBitrate);
		//MS_DEBUG_TAG(bwe, "[cst] new previousAvailableBitrate:%u(kb) transport_id_:%s", previousAvailableBitrate/(1024*8), transport_id_.c_str());
		//WARN("[cst] new previousAvailableBitrate(kb):", previousAvailableBitrate/(1024*8), "transport_id_:", transport_id_);

		// �׳��ϴο��ñ�����������
		MayEmitAvailableBitrateEvent(previousAvailableBitrate);
	}

	// Called from PacedSender in order to send probation packets.
	void TransportCongestionControlClient::SendPacket(
	  RTC::RtpPacket* packet, const webrtc::PacedPacketInfo& pacingInfo)
	{
		MS_TRACE();

		// Send the packet.
		this->listener->OnTransportCongestionControlClientSendRtpPacket(this, packet, pacingInfo);
	}

	RTC::RtpPacket* TransportCongestionControlClient::GeneratePadding(size_t size)
	{
		MS_TRACE();

		return this->probationGenerator->GetNextPacket(size);
	}

	void TransportCongestionControlClient::OnTimer(Timer* timer)
	{
		MS_TRACE();

		if (timer == this->processTimer)
		{
			// Time to call RtpTransportControllerSend::Process().
			this->rtpTransportControllerSend->Process();

			// Time to call PacedSender::Process().
			this->rtpTransportControllerSend->packet_sender()->Process();

			/* clang-format off */
			int interval = std::min(
				// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
				this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
				// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
				this->controllerFactory->GetProcessInterval().ms()
			);
			//INFO("[cst] interval:", interval);
			this->processTimer->Start(std::min<uint64_t>(
				// Depends on probation being done and WebRTC-Pacer-MinPacketLimitMs field trial.
				this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(),
				// Fixed value (25ms), libwebrtc/api/transport/goog_cc_factory.cc.
				this->controllerFactory->GetProcessInterval().ms()
			));
			//this->processTimer->Start(1000);
			/* clang-format on */

			//INFO("[timer] TimeUntilNextProcess:", this->rtpTransportControllerSend->packet_sender()->TimeUntilNextProcess(), 
			//	"GetProcessInterval:", this->controllerFactory->GetProcessInterval().ms(),
			//	"availableBitrate(kbps):", this->bitrates.availableBitrate/(1024));
			//INFO("[bwe] before MayEmitAvailableBitrateEvent �۷�����л���consumer ���Ͷ˵�ӵ�����ƶ�ʱ���ᶨʱ���������õ�������ʲ�����");
			MayEmitAvailableBitrateEvent(this->bitrates.availableBitrate);
		}
	}
} // namespace RTC
