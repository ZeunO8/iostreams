#pragma once
#include <map>
#include "Serial.hpp"
#include "tcp_iostream.hpp"
namespace iostreams
{
	struct tcp_server
	{
	public:
		using ClientTuple = std::tuple<int, std::shared_ptr<Serial>, std::shared_ptr<iostreams::streams::tcp_iostream>>;
#if defined(_WIN32)
		using SockLength = int;
#elif defined(__linux__) || defined(MACOS) || defined(IOS)
		using SockLength = socklen_t;
#endif
	private:
		int port;
		bool bitStream;
		SSL_CTX* ssl_ctx = 0;
		int server_fd = 0;
		size_t totalClients;
		std::map<size_t, ClientTuple> clientStreamMap;

	public:
		tcp_server(int port, bool bitStream = false, SSL_CTX* ssl_ctx = 0);
		~tcp_server();

	private:
		void close();

	public:
		int acceptOne(ClientTuple** out_client_tuple_ptr);
		void upgradeSSL(SSL_CTX* ssl_ctx);
	};
} // namespace iostreams
