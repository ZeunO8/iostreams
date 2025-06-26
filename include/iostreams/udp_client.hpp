#pragma once
#include "udp_iostream.hpp"
namespace iostreams
{
	struct udp_client : iostreams::streams::udp_iostream
	{
	public:
		udp_client(const std::string& host, int port);

	private:
		streams::udp_streambuf::SocketPair connect(const std::string& host, int port);
	};
} // namespace iostreams
