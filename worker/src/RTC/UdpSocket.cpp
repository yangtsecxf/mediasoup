#define MS_CLASS "RTC::UdpSocket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/UdpSocket.hpp"
#include "Logger.hpp"
#include "RTC/PortManager.hpp"
#include <string>

namespace RTC
{
	/* Instance methods. */

	UdpSocket::UdpSocket(Listener* listener, std::string& ip)
	  : // This may throw.
	    ::UdpSocket::UdpSocket(PortManager::BindUdp(ip)), listener(listener)
	{
		MS_TRACE();
	}

    // 连接三方udp服务器
    UdpSocket:: UdpSocket(Listener* listener, std::string& ip , uint16_t port)
            : // This may throw.
            ::UdpSocket::UdpSocket(PortManager::ConnectUdp(ip , port)), listener(listener), enableOtherUdp(true)
    {
        MS_TRACE();
    }


	UdpSocket::~UdpSocket()
	{
		MS_TRACE();
        if (!enableOtherUdp) {
			PortManager::UnbindUdp(this->localIp, this->localPort);
		} else {
			//
		}
	}

	void UdpSocket::UserOnUdpDatagramReceived(const uint8_t* data, size_t len, const struct sockaddr* addr)
	{
		MS_TRACE();

		if (!this->listener)
		{
			MS_ERROR("no listener set");

			return;
		}

		// Notify the reader.
		this->listener->OnUdpSocketPacketReceived(this, data, len, addr);
	}
} // namespace RTC
