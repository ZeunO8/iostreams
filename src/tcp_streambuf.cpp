#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <iostreams/tcp_streambuf.hpp>
using namespace iostreams::streams;
#if defined(_WIN32)
#define IO_DONTWAIT 0
#define IO_SHUTDOWN SD_SEND
#define IO_INVALIDSOCK1 WSANOTINITIALISED
#define IO_INVALIDSOCK2 WSANOTINITIALISED
#else
#define IO_DONTWAIT MSG_DONTWAIT
#define IO_SHUTDOWN SHUT_WR
#define IO_INVALIDSOCK1 EBADF
#define IO_INVALIDSOCK2 ENOTSOCK
#endif

tcp_streambuf::tcp_streambuf(const std::pair<int, SSL *> &fd_ssl_pair) :
	fd(std::get<0>(fd_ssl_pair)),
	ssl(std::get<1>(fd_ssl_pair))
{
}

tcp_streambuf::~tcp_streambuf()
{
	sync();
	close();
}

std::streamsize tcp_streambuf::xsgetn(char *s, std::streamsize n)
{
	// if(!wait_readable(3))
	// 	return 0;
	if (bytes_available() < n)
		return 0;

	long long __bytes__read__ = 0;

	if (ssl)
	{
		__bytes__read__ = SSL_read(ssl, s, static_cast<int>(n));
	}
	else
	{
		__bytes__read__ = recv(fd, s, static_cast<int>(n), IO_DONTWAIT);
	}

	if (__bytes__read__ > 0)
	{
		stream_empty = false;

		read_pos += __bytes__read__;
		read_length += __bytes__read__;

		return __bytes__read__;
	}

	if (__bytes__read__ == 0)
	{
		if (ssl && SSL_get_error(ssl, 0) == SSL_ERROR_ZERO_RETURN)
		{
			connection_closed = true;
			return 0;
		}

		stream_empty = true;
		return 0;
	}

#if defined(_WIN32)
	auto err = WSAGetLastError();
	if (err == WSAEWOULDBLOCK)
	{
#else
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
#endif
		stream_empty = true;
		return 0;
	}
	else
	{
		std::cout << "err: " << err << std::endl;
	}

	connection_closed = true;
	return 0;
}

std::streamsize tcp_streambuf::xsputn(const char *s, std::streamsize n)
{
	if (n > 0)
	{
		if (!wait_writable(1))
			return 0;
		size_t sent;
		if (ssl)
			sent = SSL_write(ssl, s, n);
		else
			sent = send(fd, s, n, 0);

		if (sent <= 0)
			return -1; // indicate error

		write_pos += sent;
		write_length += sent;
		return static_cast<std::streamsize>(sent);
	}
	return 0;
}

// int tcp_streambuf::underflow()
// {
// 	long long __bytes__read__ = 0;

// 	char c = 0;

// 	if (ssl)
// 	{
// 		__bytes__read__ = SSL_read(ssl, &c, 1);
// 	}
// 	else
// 	{
// 		// use non-blocking recv with MSG_DONTWAIT
// 		__bytes__read__ = recv(fd, &c, 1, IO_DONTWAIT);
// 	}

// 	if (__bytes__read__ > 0)
// 	{
// 		stream_empty = false;

// 		read_pos++;

// 		return traits_type::to_int_type(c);
// 	}

// 	// handle empty but still open
// 	if (__bytes__read__ == 0)
// 	{
// 		// For SSL, SSL_read(…) == 0 means closed unless SSL_pending() > 0
// 		if (ssl && SSL_get_error(ssl, 0) == SSL_ERROR_ZERO_RETURN)
// 		{
// 			connection_closed = true;
// 			return traits_type::eof();
// 		}

// 		// Otherwise: no data, still open
// 		stream_empty = true;
// 		return traits_type::eof();
// 	}

// #if defined(_WIN32)
// 	auto err = WSAGetLastError();
// 	if (err == WSAEWOULDBLOCK)
// 	{
// #else
// 	if (errno == EAGAIN || errno == EWOULDBLOCK)
// 	{
// #endif
// 		// non-blocking: no data yet
// 		stream_empty = true;
// 		return traits_type::eof();
// 	}

// 	// real error → closed
// 	connection_closed = true;
// 	return traits_type::eof();
// }

// int tcp_streambuf::overflow(int c)
// {
// 	write_pos++;
// 	write_length++;
// 	size_t sent;
// 	if (ssl)
// 		sent = SSL_write(ssl, (const char *)&c, 1);
// 	else
// 		sent = send(fd, (const char *)&c, 1, IO_DONTWAIT);
// 	if (sent <= 0)
// 		return traits_type::eof();
// 	return c;
// }
// std::streamsize tcp_streambuf::showmanyc()
// {
// 	if (gptr() < egptr())
// 		return egptr() - gptr();

// 	if (stream_empty && !connection_closed)
// 		return 0; // empty, but not EOF

// 	if (connection_closed)
// 		return -1; // EOF

// 	return 0;
// }

// Seek relative
tcp_streambuf::pos_type tcp_streambuf::seekoff(off_type off, std::ios_base::seekdir way,
											   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
{
	if (which & std::ios_base::out)
	{
		switch (way)
		{
		case std::ios_base::beg:
			write_pos = off;
			break;
		case std::ios_base::cur:
			write_pos += off;
			break;
		case std::ios_base::end:
			write_pos = write_length + off;
			break;
		}
		if (write_pos > write_length)
			write_length = write_pos;
		return write_pos;
	}
	if (which & std::ios_base::in)
	{
		switch (way)
		{
		case std::ios_base::beg:
			read_pos = off;
			break;
		case std::ios_base::cur:
			read_pos += off;
			break;
		case std::ios_base::end:
			read_pos = read_length + off;
			break;
		}
		return read_pos;
	}
	return -1;
}

// Seek absolute
tcp_streambuf::pos_type tcp_streambuf::seekpos(pos_type sp,
											   std::ios_base::openmode which = std::ios_base::in | std::ios_base::out)
{
	return seekoff(off_type(sp), std::ios_base::beg, which);
}

// int tcp_streambuf::sync()
// {
// 	return 0;
// }

socket_wait_result tcp_streambuf::wait_for_socket(int timeout_ms, bool want_read, bool want_write)
{
	fd_set readset;
	fd_set writeset;
	fd_set exceptset;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&exceptset);

	if (want_read)
	{
		FD_SET(fd, &readset);
	}
	if (want_write)
	{
		FD_SET(fd, &writeset);
	}
	FD_SET(fd, &exceptset);

	timeval tv;
	timeval *tvp = nullptr;
	if (timeout_ms >= 0)
	{
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		tvp = &tv;
	}

	int res = select(1, &readset, &writeset, &exceptset, tvp);
	socket_wait_result out;
	if (res < 0)
	{
#if defined(_WIN32) || defined(_WIN64)
		int err = WSAGetLastError();
		if (err == IO_INVALIDSOCK1 || err == IO_INVALIDSOCK2)
			return out;
		std::cerr << std::system_error(err, std::system_category(), "select failed").what() << std::endl;
#else
		std::cerr << std::system_error(errno, std::system_category(), "select failed").what() << std::endl;
#endif
		return out;
	}

	if (res > 0)
	{
		out.readable = (FD_ISSET(fd, &readset) != 0);
		out.writable = (FD_ISSET(fd, &writeset) != 0);
		out.excepted = (FD_ISSET(fd, &exceptset) != 0);
	}
	return out;
}

socket_wait_result wait_for_socket(int fd, int timeout_ms, bool want_read, bool want_write)
{
	fd_set readset;
	fd_set writeset;
	fd_set exceptset;
	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&exceptset);

	if (want_read)
	{
		FD_SET(fd, &readset);
	}
	if (want_write)
	{
		FD_SET(fd, &writeset);
	}
	FD_SET(fd, &exceptset);

	timeval tv;
	timeval *tvp = nullptr;
	if (timeout_ms >= 0)
	{
		tv.tv_sec = timeout_ms / 1000;
		tv.tv_usec = (timeout_ms % 1000) * 1000;
		tvp = &tv;
	}

	int res = select(1, &readset, &writeset, &exceptset, tvp);
	socket_wait_result out;
	if (res < 0)
	{
#if defined(_WIN32) || defined(_WIN64)
		int err = WSAGetLastError();
		if (err == IO_INVALIDSOCK1 || err == IO_INVALIDSOCK2)
			return out;
		std::cerr << std::system_error(err, std::system_category(), "select failed").what() << std::endl;
#else
		std::cerr << std::system_error(errno, std::system_category(), "select failed").what() << std::endl;
#endif
		return out;
	}

	if (res > 0)
	{
		out.readable = (FD_ISSET(fd, &readset) != 0);
		out.writable = (FD_ISSET(fd, &writeset) != 0);
		out.excepted = (FD_ISSET(fd, &exceptset) != 0);
	}
	return out;
}

bool tcp_streambuf::wait_readable(int timeout_ms)
{
	return wait_for_socket(timeout_ms, true, false).readable;
}

bool tcp_streambuf::wait_writable(int timeout_ms)
{
	return wait_for_socket(timeout_ms, false, true).writable;
}

bool wait_readable(int fd, int timeout_ms)
{
	return wait_for_socket(fd, timeout_ms, true, false).readable;
}

bool wait_writable(int fd, int timeout_ms)
{
	return wait_for_socket(fd, timeout_ms, false, true).writable;
}

int tcp_streambuf::bytes_available()
{
#ifdef _WIN32
	u_long count = 0;
	if (ioctlsocket(fd, FIONREAD, &count) != 0)
		throw std::runtime_error("ioctlsocket(FIONREAD) failed");
	return static_cast<int>(count);
#else
	int count = 0;
	if (ioctl(fd, FIONREAD, &count) < 0)
		throw std::runtime_error("ioctl(FIONREAD) failed");
	return count;
#endif
}

int tcp_streambuf::send_buffer_free()
{
#ifdef _WIN32
	// Windows does not expose SO_SNDBUF free space directly.
	// We approximate by checking total send buffer size (SO_SNDBUF)
	// minus queued bytes (SIO_TCP_INFO not available everywhere).
	// Best effort approach is to query SO_SNDBUF.
	int bufsize = 0;
	int optlen = sizeof(bufsize);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char *)&bufsize, &optlen) != 0)
		throw std::runtime_error("getsockopt(SO_SNDBUF) failed");
	return bufsize;
#else
	int pending = 0;
	if (ioctl(fd, TIOCOUTQ, &pending) < 0)
		throw std::runtime_error("ioctl(TIOCOUTQ) failed");
	int bufsize = 0;
	socklen_t optlen = sizeof(bufsize);
	if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &bufsize, &optlen) < 0)
		throw std::runtime_error("getsockopt(SO_SNDBUF) failed");
	return bufsize - pending;
#endif
}

bool tcp_streambuf::close()
{
	if (ctx_fd_closed)
	{
		return false;
	}
	if (ssl)
	{
		int ret = SSL_shutdown(ssl);
		if (ret == 0)
			SSL_shutdown(ssl);
		SSL_free(ssl);
	}
	close_socket(fd);
	ctx_fd_closed = true;
	return true;
}
bool tcp_streambuf::close_socket(int fd)
{
	if (fd >= 0)
	{
		shutdown(fd, IO_SHUTDOWN);
		static char c;
		for (;::wait_readable(fd, 1);)
		{
			int res = recv(fd, &c, 1, IO_DONTWAIT);
			if (res < 0)
				break;
			if (!res)
				break;
		}
#ifdef _WIN32
		::closesocket(fd);
#else
		::close(fd);
#endif
		return true;
	}
	return false;
}