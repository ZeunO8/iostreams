#pragma once
#include "udp_iostream.hpp"
namespace iostreams
{
	struct udpmc_receiver : iostreams::streams::udp_istream
	{
	public:
		udpmc_receiver(const std::string& host, int port);

	private:
		streams::udp_streambuf::SocketPair bind(const std::string& host, int port);
	};
} // namespace iostreams
