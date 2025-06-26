#include <iostreams/socket_init.hpp>
#include <iostreams/platform.hpp>
using namespace iostreams;
bool socket_init::initialized =
#ifdef _WIN32
	false;
#else
	true;
#endif
void socket_init::initialize()
{
	if (initialized)
		return;
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		throw std::runtime_error("WSAStartup failed");
	}
#endif
	SSL_library_init();
	SSL_load_error_strings();
	OpenSSL_add_all_algorithms();
	initialized = true;
}
