#include <iostreams/p2p.hpp>
#include <iostreams/http_client.hpp>
using namespace iostreams;
p2p::HostInfo p2p::host_info_factory::create()
{
    HostInfo info;
    auto response = iostreams::http::http_client::restSync("GET", "https://ipinfo.io/loc");
    std::string latlngString(response.body.second.get(), response.body.second.get() + response.body.first);
    if (latlngString.size() && latlngString[latlngString.size() - 1] == '\n')
    {
        latlngString.erase(latlngString.end() - 1);
    }
    auto comPos = latlngString.find(',');
    if (comPos == std::string::npos)
    {
        return info;
    }
    auto latString = latlngString.substr(0, comPos);
    auto lngString = latlngString.substr(comPos + 1, latlngString.size() - 1);
    info.latitude = std::stold(latString);
    info.longitude = std::stold(lngString);
    return info;
}
p2p::p2p(const std::string& announceIP):
    announceIP(announceIP)
    
{
    startAnnounce();
}
bool p2p::doingAnnounce()
{
    return !!announceReceiver;
}
void p2p::startAnnounce()
{
    announceReceiver = std::make_shared<udpmc_receiver>(announceIP, 8427);
    char _char = 0;
    Serial serial(*announceReceiver);
    serial >> _char;
    std::cout << "Received char: " << _char;
}
void p2p::closeAnnounce()
{
    return;
}