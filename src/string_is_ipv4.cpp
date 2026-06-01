#include <iostreams/string_is_ipv4.hpp>
#include <array>
bool iostreams::string_is_ipv4(const std::string& str)
{
    if (str.empty())
        return false;
    auto strData = str.data();
    auto strSize = str.size();
    std::array<std::string, 4> segments;
    int segmentIndex = 0;
    for (size_t index = 0; index < strSize; index++)
    {
        unsigned char ch = strData[index];
        if (ch >= '0' && ch <= '9')
        {
            segments[segmentIndex] += ch;
            if (segments[segmentIndex].size() > 3)
                return false;
            continue;
        }
        else if (ch == '.')
        {
            if (segmentIndex >= 3)
                return false;
            ++segmentIndex;
            continue;
        }
        return false;
    }
    if (segmentIndex != 3)
        return false;
    for (char segindx = 0; segindx < 4; segindx++)
    {
        if (segments[segindx].empty())
            return false;
        auto segy = std::stoll(segments[segindx]);
        if (segy > 255 || segy < 0)
        {
            return false;
        }
    }
    return true;
}