#include <iostreams/system_dns.hpp>
#include <iostreams/socket_init.hpp>
using namespace iostreams::dns::system;
std::vector<std::string> system_dns::queryA(const std::string& hostname) { return queryFamily(AF_INET, hostname); }
std::vector<std::string> system_dns::queryAAAA(const std::string& hostname)
{
#if defined(__linux__) || defined(MACOS)
	return queryFamily(AF_INET6, hostname);
#elif defined(_WIN32)
	return queryFamily(AF_INET, hostname);
#endif
}
std::vector<std::string> system_dns::queryFamily(const int& family, const std::string& hostname)
{
	std::vector<std::string> ips;
    iostreams::socket_init::initialize();
	hostent* host = gethostbyname(hostname.c_str());
    if (!host)
    {
        return ips;
    }
    for (auto hostIndex = 0; host->h_addr_list[hostIndex]; hostIndex++)
    {
        in_addr addr;
        memcpy(&addr, host->h_addr_list[hostIndex], sizeof(in_addr));
        ips.push_back(inet_ntoa(addr));
    }
	return ips;
}
