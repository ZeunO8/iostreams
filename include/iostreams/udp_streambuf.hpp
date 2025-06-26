#pragma once
#include <streambuf>
#include <vector>
#include "platform.hpp"
namespace iostreams::streams
{
	class udp_streambuf : public std::streambuf
	{
	public:
		size_t readSize = 1;
		size_t readIndex = 0;
		sockaddr_in addr;
#if defined(_WIN32)
		using SocketLength = int;
#elif defined(__linux__) || defined(MACOS)
		using SocketLength = socklen_t;
#endif
		inline static SocketLength addr_len = sizeof(sockaddr_in);
#if defined(_WIN32)
		using SocketIdentifier = SOCKET;
#elif defined(__linux__) || defined(MACOS)
		using SocketIdentifier = int;
#endif
		using SocketPair = std::pair<SocketIdentifier, sockaddr_in>;
		explicit udp_streambuf(const SocketPair& fd_addr_pair);

		~udp_streambuf();

		void add_received_data(const char* data, size_t size);

	protected:
	
		int underflow() override;

		int overflow(int c = traits_type::eof()) override;

		int sync() override;

	private:
		SocketIdentifier fd;
		std::vector<char> gbuffer;
		std::vector<char> pbuffer;
		void close();
	};
} // namespace iostreams::streams
