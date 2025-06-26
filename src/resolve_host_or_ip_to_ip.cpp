#include <iostreams/resolve_host_or_ip_to_ip.hpp>
#include <iostreams/string_is_ipv4.hpp>
#include <iostreams/system_dns.hpp>
#include <stdexcept>
std::string iostreams::resolve_host_or_ip_to_ip(const std::string& host)
{
    std::string ip;
    if (iostreams::string_is_ipv4(host))
    {
        ip = host;
    }
    else
    {
        auto ips = iostreams::dns::system::system_dns::queryA(host);
        if (ips.size())
        {
            ip = ips[0];
        }
        else
        {
            throw std::runtime_error("Could not find ip for host: " + host);
        }
    }
    return ip;
}