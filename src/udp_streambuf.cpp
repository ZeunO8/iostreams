#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <iostreams/udp_streambuf.hpp>
#include <cerrno>
using namespace iostreams::streams;
udp_streambuf::udp_streambuf(const SocketPair& fd_addr_pair) :
		fd(std::get<0>(fd_addr_pair)), addr(std::get<1>(fd_addr_pair)), gbuffer(4196)
{
	setg(gbuffer.data(), gbuffer.data(), gbuffer.data());
}
udp_streambuf::~udp_streambuf()
{
	sync();
	close();
}
void udp_streambuf::add_received_data(const char* data, size_t size)
{
	if (gbuffer.size() < size) {
		gbuffer.resize(size);
	}
	std::copy(data, data + size, gbuffer.begin());
	setg(gbuffer.data(), gbuffer.data(), gbuffer.data() + size);
}
int udp_streambuf::underflow()
{
	if (gptr() < egptr())
	{
		return traits_type::to_int_type(*gptr());
	}

	SocketLength client_len = sizeof(addr);
	
	long long bytes_received = recvfrom(fd, gbuffer.data(), gbuffer.size(), 0, (struct sockaddr*)&addr, &client_len);

	if (bytes_received > 0)
	{
		setg(gbuffer.data(), gbuffer.data(), gbuffer.data() + bytes_received);
		return traits_type::to_int_type(*gptr());
	}
	else if (bytes_received == 0)
	{
		return traits_type::eof();
	}
	else
	{
		int error_value = errno;
		if (error_value == EAGAIN || error_value == EWOULDBLOCK)
		{
			return traits_type::eof();
		}
		else
		{
			perror("recvfrom failed in underflow");
			return traits_type::eof();
		}
	}
}
int udp_streambuf::overflow(int c)
{
	char* pbase_ptr = pbase();
	char* pptr_ptr = pptr();

	size_t num_to_send = pptr_ptr - pbase_ptr;

	if (num_to_send > 0)
	{
		long long sent = sendto(fd, pbase_ptr, num_to_send, 0, (struct sockaddr*)&addr, addr_len);

		if (sent == -1)
		{
			int error_value = errno;
			return traits_type::eof();
		}
		setp(pbase_ptr, epptr());
	}

	if (!traits_type::eq_int_type(c, traits_type::eof()))
	{
		if (pptr() < epptr()) {
			*pptr() = traits_type::to_char_type(c);
			pbump(1);
			return c;
		}
		else
		{
			long long sent_one = sendto(fd, reinterpret_cast<const char*>(&c), 1, 0, (struct sockaddr*)&addr, addr_len);
			if (sent_one == -1)
			{
				int error_value = errno;
				return traits_type::eof();
			}
			return c;
		}
	}

	return traits_type::not_eof(c);
}
int udp_streambuf::sync()
{
	if (overflow() == traits_type::eof())
	{
		return -1;
	}
	return 0;
}
void udp_streambuf::close()
{
	if (fd >= 0)
	{
#ifdef _WIN32
		::closesocket(fd);
#else
		::close(fd);
#endif
	}
}
