#include <iostreams/tcp_iostream.hpp>
using namespace iostreams::streams;
tcp_istream::tcp_istream(const std::pair<int, SSL*>& fd_ssl_pair) : std::istream(&buf), buf(fd_ssl_pair) {}
bool tcp_istream::close()
{
    return buf.close();
}
tcp_ostream::tcp_ostream(const std::pair<int, SSL*>& fd_ssl_pair) : std::ostream(&buf), buf(fd_ssl_pair) {}
bool tcp_ostream::close()
{
    return buf.close();
}
tcp_iostream::tcp_iostream(const std::pair<int, SSL*>& fd_ssl_pair) : std::iostream(&buf), buf(fd_ssl_pair) {}
bool tcp_iostream::close()
{
    return buf.close();
}