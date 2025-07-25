#pragma once
#include <streambuf>
#include <vector>
#include "platform.hpp"
namespace iostreams::streams
{
	class tcp_streambuf : public std::streambuf
	{
	public:
		size_t readSize = 1;
		size_t readIndex = 0; 

		explicit tcp_streambuf(const std::pair<int, SSL*>& fd_ssl_pair, std::size_t buffer_size = 4096);

		~tcp_streambuf();

		void SSLUpgrade(SSL* sslPointer);

	protected:
		int underflow() override;

		int overflow(int c = traits_type::eof()) override;

		int sync() override;

	private:
#if defined(_WIN32)
		using SocketIdentifier = SOCKET;
#elif defined(__linux__) || defined(MACOS) || defined(IOS)
		using SocketIdentifier = int;
#endif
		SocketIdentifier fd;
		std::vector<char> gbuffer;
		std::vector<char> pbuffer;
		SSL* ssl;
		void close();
	};
} // namespace zg
