#include <iostreams/system_dns.hpp>
#include <iostreams/socket_init.hpp>
#include <cstring>
using namespace iostreams::dns::system;
std::vector<std::string> system_dns::queryA(const std::string& hostname) { return queryFamily(AF_INET, hostname); }
std::vector<std::string> system_dns::queryAAAA(const std::string& hostname)
{
#if defined(__linux__) || defined(__APPLE__) || defined(MACOS) || defined(IOS)
	return queryFamily(AF_INET6, hostname);
#elif defined(_WIN32)
	return queryFamily(AF_INET, hostname);
#endif
	return {};
}
std::vector<std::string> system_dns::queryFamily(const int& family, const std::string& hostname)
{
	std::vector<std::string> ips;
    iostreams::socket_init::initialize();

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = family;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    int ret = getaddrinfo(hostname.c_str(), nullptr, &hints, &result);
    if (ret != 0 || result == nullptr)
    {
        return ips;
    }

    char ipstr[INET6_ADDRSTRLEN];
    for (addrinfo* p = result; p != nullptr; p = p->ai_next)
    {
        if (family == AF_INET)
        {
            sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            inet_ntop(AF_INET, &ipv4->sin_addr, ipstr, sizeof(ipstr));
        }
        else
        {
            sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(p->ai_addr);
            inet_ntop(AF_INET6, &ipv6->sin6_addr, ipstr, sizeof(ipstr));
        }
        ips.push_back(ipstr);
    }

    freeaddrinfo(result);
	return ips;
}
