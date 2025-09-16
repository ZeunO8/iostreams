#include <iostreams/tcp_streambuf.hpp>
using namespace iostreams::streams;
#if defined(_WIN32)
#define IO_DONTWAIT 0
#else
#define IO_DONTWAIT MSG_DONTWAIT
#endif
tcp_streambuf::tcp_streambuf(const std::pair<int, SSL *> &fd_ssl_pair, std::size_t buffer_size) : fd(std::get<0>(fd_ssl_pair)), /*gbuffer(buffer_size), */ pbuffer(buffer_size), ssl(std::get<1>(fd_ssl_pair))
{
	setg(gbuffer.data(), gbuffer.data(), gbuffer.data());
	setp(pbuffer.data(), pbuffer.data() + pbuffer.size());
}
tcp_streambuf::~tcp_streambuf()
{
	sync();
	close();
}
int tcp_streambuf::underflow()
{
	// still have unread data in buffer
	if (gptr() < egptr())
		return traits_type::to_int_type(*gptr());

	// ensure capacity for at least readSize more bytes
	if ((int64_t)gbuffer.size() - readIndex < readSize)
		gbuffer.resize(readIndex + readSize);

	long long __bytes__read__ = 0;

	if (ssl)
	{
		__bytes__read__ = SSL_read(ssl, gbuffer.data() + readIndex, readSize);
	}
	else
	{
		// use non-blocking recv with MSG_DONTWAIT
		__bytes__read__ = recv(fd, gbuffer.data() + readIndex, readSize, IO_DONTWAIT);
	}

	if (__bytes__read__ > 0)
	{
		stream_empty = false;

		// set buffer pointers correctly for new data
		char *base = gbuffer.data();
		char *start = base + readIndex;
		char *end = start + __bytes__read__;

		// move readIndex forward after consuming the new block
		readIndex += __bytes__read__;

		setg(base, start, end);
		return traits_type::to_int_type(*gptr());
	}

	// handle empty but still open
	if (__bytes__read__ == 0)
	{
		setg(gbuffer.data(), gbuffer.data() + readIndex, gbuffer.data() + readIndex);

		// For SSL, SSL_read(…) == 0 means closed unless SSL_pending() > 0
		if (ssl && SSL_get_error(ssl, 0) == SSL_ERROR_ZERO_RETURN)
		{
			connection_closed = true;
			return traits_type::eof();
		}

		// Otherwise: no data, still open
		stream_empty = true;
		return traits_type::eof();
	}

#if defined(_WIN32)
	auto err = WSAGetLastError();
	if (err == WSAEWOULDBLOCK)
	{
#else
	if (errno == EAGAIN || errno == EWOULDBLOCK)
	{
#endif
		// non-blocking: no data yet
		stream_empty = true;
		return traits_type::eof();
	}

	// real error → closed
	connection_closed = true;
	return traits_type::eof();
}

int tcp_streambuf::overflow(int c)
{
	if (sync() == -1)
		return traits_type::eof();

	if (c != traits_type::eof())
	{
		*pptr() = c;
		pbump(1);
	}
	return c;
}
std::streamsize tcp_streambuf::showmanyc()
{
	if (gptr() < egptr())
		return egptr() - gptr();

	if (stream_empty && !connection_closed)
		return 0; // empty, but not EOF

	if (connection_closed)
		return -1; // EOF

	return 0;
}

std::streambuf::pos_type tcp_streambuf::seekoff(off_type off, std::ios_base::seekdir dir,
												std::ios_base::openmode which)
{
	if (!(which & std::ios_base::in))
		return pos_type(off_type(-1));

	// current position relative to beginning of get area
	off_type base = gptr() - eback();

	if (dir == std::ios_base::cur)
	{
		return base + off;
	}
	else if (dir == std::ios_base::beg)
	{
		return off;
	}
	else if (dir == std::ios_base::end)
	{
		return (egptr() - eback()) + off;
	}
	return pos_type(off_type(-1));
}

std::streambuf::pos_type tcp_streambuf::seekpos(pos_type sp, std::ios_base::openmode which)
{
	return seekoff(off_type(sp), std::ios_base::beg, which);
}
int tcp_streambuf::sync()
{
	auto _pbase = pbase();
	auto _pptr = pptr();
	size_t n = _pptr - _pbase;
	if (n > 0)
	{
		size_t sent;
		if (ssl)
			sent = SSL_write(ssl, _pbase, n);
		else
			sent = send(fd, _pbase, n, 0);
		if (sent <= 0)
			return -1;

		pbump(-n);
	}
	gbuffer.clear();
	readIndex = 0;
	setg(gbuffer.data(), gbuffer.data() + readIndex, gbuffer.data() + readIndex);
	pbuffer.clear();
	return 0;
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
#ifdef _WIN32
		::closesocket(fd);
#else
		::close(fd);
#endif
		return true;
	}
	return false;
}