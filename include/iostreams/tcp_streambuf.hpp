#pragma once
#include <streambuf>
#include <vector>
#include "platform.hpp"
namespace iostreams::streams
{
	class tcp_streambuf : public std::streambuf
	{
	public:
		bool ctx_fd_closed = false;
		bool connection_closed = false;
		bool stream_empty = true;

        std::streamoff write_pos = 0;    // current write position
        std::streamoff read_pos = 0;     // current read position
        std::streamoff write_length = 0; // maximum written
        std::streamoff read_length = 0; // maximum read

		explicit tcp_streambuf(const std::pair<int, SSL*>& fd_ssl_pair, std::size_t buffer_size = 4096);

		~tcp_streambuf();

		void SSLUpgrade(SSL* sslPointer);

	protected:
		
        // Read multiple characters
        std::streamsize xsgetn(char* s, std::streamsize n) override;

		std::streamsize xsputn(const char* s, std::streamsize n) override;

		int underflow() override;

		int overflow(int c = traits_type::eof()) override;

		std::streamsize showmanyc() override;

		pos_type seekoff(off_type off, std::ios_base::seekdir dir,
						std::ios_base::openmode which) override;

		pos_type seekpos(pos_type sp, std::ios_base::openmode which) override;

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

	public:
		bool close();
		static bool close_socket(int);
	};
} // namespace zg
