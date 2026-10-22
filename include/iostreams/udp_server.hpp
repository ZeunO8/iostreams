#pragma once
#include <map>
#include <memory>
#include "Serial.hpp"
#include "udp_iostream.hpp"
namespace iostreams
{
	struct udp_server
	{
	public:
		using IOStreamPointer = std::shared_ptr<iostreams::streams::udp_iostream>;
#if defined(_WIN32)
		using SockLength = int;
#elif defined(__linux__) || defined(__APPLE__) || defined(MACOS) || defined(IOS)
		using SockLength = socklen_t;
#endif
	private:
		int port;
		bool bitStream;
		int server_fd = 0;
		unsigned int m_nonBlockMicroSecTimeout = 0;
		std::map<std::pair<uint32_t, uint16_t>, IOStreamPointer> clientStreams;

	public:
		udp_server(int port, bool bitStream = false);
	private:
		void close();
	public:
		IOStreamPointer receiveOne(bool nonBlock = false, unsigned int nonBlockMicroSecTimeout = 10);
	};
}
