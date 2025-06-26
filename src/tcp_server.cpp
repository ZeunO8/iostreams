#include <iostreams/socket_init.hpp>
#include <iostreams/tcp_server.hpp>
using namespace iostreams;
#define BACKLOG 5
tcp_server::tcp_server(int port, bool bitStream, SSL_CTX* ssl_ctx) :
		port(port), bitStream(bitStream), ssl_ctx(ssl_ctx)
{
	socket_init::initialize();
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	sockaddr_in server_addr{};
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(port);
	if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		close();
		throw std::runtime_error("Bind failed");
	}
	if (listen(server_fd, BACKLOG) == -1)
	{
		close();
		throw std::runtime_error("Listen failed");
	}
}
tcp_server::~tcp_server()
{
	close();
}
void tcp_server::close()
{
#ifdef _WIN32
	::closesocket(server_fd);
#else
	::close(server_fd);
#endif
}
tcp_server::ClientTuple& tcp_server::acceptOne()
{
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(sockaddr_in));
	SockLength client_len = sizeof(client_addr);
	int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
	if (client_fd == -1)
	{
		close();
#ifdef _WIN32
		int error = WSAGetLastError();
		throw std::runtime_error("Accept failed. Error code: " + std::to_string(error));
#else
		throw std::runtime_error("Accept failed: " + std::string(std::strerror(errno)));
#endif
	}
	auto id = ++totalClients;
	SSL* ssl = 0;
	if (ssl_ctx)
	{
		ssl = SSL_new(ssl_ctx);
		SSL_set_fd(ssl, client_fd);
		int ret = 0;
		if ((ret = SSL_accept(ssl)) < 0)
		{
			int ssl_error = SSL_get_error(ssl, ret);
			std::string errStr;
			unsigned long err = 0;
			while ((err = ERR_get_error()) != 0)
			{
				errStr += "OpenSSL Error: " + std::string(ERR_reason_error_string(err)) + "\n";
			}
			std::cerr << "SSL failed to accept: " << errStr;
		}
	}
	auto client_iostream = std::make_shared<iostreams::streams::tcp_iostream>(std::pair<int, SSL*>(client_fd, ssl));
	auto client_serial = std::make_shared<Serial>(*client_iostream, bitStream);
	auto& clientTuple = (clientStreamMap[id] = ClientTuple(client_fd, client_serial, client_iostream));
	return clientTuple;
}
