#pragma once
#include "tcp_iostream.hpp"
namespace iostreams
{
	struct tcp_client : iostreams::streams::tcp_iostream
	{
	public:
		tcp_client(const std::string& host, int port, SSL_CTX* ssl_ctx = 0, bool verifyCerts = true, bool enable_non_blocking = false);
		~tcp_client();

		void SSLUpgrade(const SSLPair& sslPair);

	private:
		std::pair<int, SSL*> connect(const std::string& host, int port, SSL_CTX* ssl_ctx, bool verifyCerts, bool enable_non_blocking);
		SSL_CTX* ssl_ctx = 0;
	};
} // namespace iostreams
