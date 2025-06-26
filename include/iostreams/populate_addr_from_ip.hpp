#pragma once
#include "platform.hpp"
namespace iostreams
{
    void populate_addr_from_ip(sockaddr_in& addr, const std::string& ip);
}