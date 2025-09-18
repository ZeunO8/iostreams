#include <iostreams/socket_init.hpp>
#include <iostreams/tcp_server.hpp>
#include <cstring>
using namespace iostreams;
#define BACKLOG 5
tcp_server::tcp_server(int port, bool bitStream, SSL_CTX* ssl_ctx, bool enable_non_blocking) :
		port(port), bitStream(bitStream), ssl_ctx(ssl_ctx)
{
	socket_init::initialize();
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd == -1)
	{
		throw std::runtime_error("Socket creation failed");
	}
	// if (enable_non_blocking)
	// {
		if (!streams::SetNonBlocking(server_fd))
		{
			close();
			throw std::runtime_error("SetNonBlocking failed");
		}
	// }
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
bool tcp_server::close()
{
	std::lock_guard lock(mutex);
	if (fd_closed)
		return false;
	streams::tcp_streambuf::close_socket(server_fd);
	fd_closed = true;
	return true;
}

#if defined(_WIN32)
#ifndef socklen_t
#define socklen_t int
#endif
#endif

int accept_with_timeout(int listen_fd, struct sockaddr *addr, socklen_t *addrlen, int timeout_ms) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(listen_fd, &rfds);

    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    int ret = select(1, &rfds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(listen_fd, &rfds)) {
        return accept(listen_fd, addr, addrlen);
    } else if (ret == 0) {
        // Timeout
        errno = EAGAIN;
        return -1;
    } else {
        // Error
        return -1;
    }
}

int tcp_server::acceptOne(ClientTuple** out_client_tuple_ptr)
{
	sockaddr_in client_addr;
	memset(&client_addr, 0, sizeof(sockaddr_in));
	SockLength client_len = sizeof(client_addr);
	int client_fd = 0;
	{
		std::lock_guard lock(mutex);
		if (fd_closed)
			return -1;
		client_fd = accept_with_timeout(server_fd, (struct sockaddr*)&client_addr, &client_len, 175);
		if (client_fd == -1)
		{
	#ifdef _WIN32
			int error = WSAGetLastError();
			// throw std::runtime_error("Accept failed. Error code: " + std::to_string(error));
	#else
			// throw std::runtime_error("Accept failed: " + std::string(std::strerror(errno)));
	#endif
			if (out_client_tuple_ptr)
				(*out_client_tuple_ptr) = nullptr;
			return -1;
		}
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
	if (out_client_tuple_ptr)
		(*out_client_tuple_ptr) = &clientTuple;
	return client_fd;
}
