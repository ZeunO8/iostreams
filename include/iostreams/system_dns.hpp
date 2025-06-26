#pragma once
#include "platform.hpp"
#include <vector>
#include <string>
namespace iostreams::dns::system
{
    struct system_dns
    {
        static std::vector<std::string> queryA(const std::string& hostname);
        static std::vector<std::string> queryAAAA(const std::string& hostname);
        static std::vector<std::string> queryFamily(const int &family, const std::string& hostname);
    };  
}