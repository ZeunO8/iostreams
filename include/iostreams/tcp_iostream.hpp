#pragma once
#include <istream>
#include <ostream>
#include "tcp_streambuf.hpp"
#include "openssl/ssl.h"
#include "openssl/err.h"
using SSLPair = std::pair<SSL_CTX *, SSL *>;
namespace iostreams::streams
{
	class tcp_istream : public std::istream
	{
	public:
		explicit tcp_istream(const std::pair<int, SSL *> &fd_ssl_pair);

		void SSLUpgrade(SSL *sslPointer);
		bool close();

	private:
		tcp_streambuf buf;
	};

	class tcp_ostream : public std::ostream
	{
	public:
		explicit tcp_ostream(const std::pair<int, SSL *> &fd_ssl_pair);

		void SSLUpgrade(SSL *sslPointer);
		bool close();

	private:
		tcp_streambuf buf;
	};

	class tcp_iostream : public std::iostream
	{
	public:
		explicit tcp_iostream(const std::pair<int, SSL *> &fd_ssl_pair);

		void SSLUpgrade(SSL *sslPointer);
		bool close();

	private:
		tcp_streambuf buf;
	};

	inline static bool SetNonBlocking(int fd)
	{
#if defined(_WIN32)
		u_long mode = 1;
		if (ioctlsocket(fd, FIONBIO, &mode) != 0)
		{
			return false;
		}
#else
		int flags = fcntl(fd, F_GETFL, 0);
		if (flags == -1)
		{
			return false;
		}
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		{
			return false;
		}
#endif
		return true;
	}
} // namespace zg
