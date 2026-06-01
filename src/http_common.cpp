#include <iostreams/http_common.hpp>
#include <iostreams/Serial.hpp>
#include <algorithm>
#include <cstring>

template <>
Serial& deserialize(Serial& serial, iostreams::http::http_response& response)
{
    std::string line;
    char ch;

    auto read_char = [&]() -> char {
        char c = 0;
        serial.readBytes(&c, 1);
        return c;
    };

    auto read_line = [&]() -> std::string {
        std::string result;
        while (true)
        {
            ch = read_char();
            if (ch == '\r')
                continue;
            if (ch == '\n')
                break;
            result += ch;
        }
        return result;
    };

    std::string status_line = read_line();

    size_t space1 = status_line.find(' ');
    if (space1 == std::string::npos)
        return serial;
    std::string protocol_version = status_line.substr(0, space1);

    size_t slash = protocol_version.find('/');
    if (slash != std::string::npos)
    {
        response.protocol = protocol_version.substr(0, slash + 1);
        response.version = protocol_version.substr(slash + 1);
    }
    else
    {
        response.protocol = protocol_version;
        response.version = "";
    }

    size_t space2 = status_line.find(' ', space1 + 1);
    if (space2 == std::string::npos)
        return serial;
    response.statusCode = status_line.substr(space1 + 1, space2 - space1 - 1);
    response.statusText = status_line.substr(space2 + 1);

    while (true)
    {
        std::string header_line = read_line();
        if (header_line.empty())
            break;

        size_t colon = header_line.find(':');
        if (colon != std::string::npos)
        {
            std::string key = header_line.substr(0, colon);
            std::string value = header_line.substr(colon + 1);
            if (!value.empty() && value[0] == ' ')
                value = value.substr(1);
            response.headers[key] = value;
        }
    }

    auto contentLengthIter = response.headers.find("Content-Length");
    if (contentLengthIter != response.headers.end() && !contentLengthIter->second.empty())
    {
        size_t contentLength = std::stoll(contentLengthIter->second);
        std::shared_ptr<int8_t> body((int8_t*)malloc(contentLength + 1), free);
        memset(body.get(), 0, contentLength + 1);
        serial.readBytes((char*)body.get(), contentLength);
        response.body = {contentLength, body};
    }

    return serial;
}

template<>
Serial& serialize(Serial& serial, const iostreams::http::http_request& request)
{
    std::string header;
    header += request.verb + " " + request.path + " HTTP/1.1\r\n";
    header += "Host: " + request.host + "\r\n";
    for (auto& headerPair : request.headers)
    {
        header += headerPair.first + ": " + headerPair.second + "\r\n";
    }
    header += "Content-Length: " + std::to_string(request.body.first) + "\r\n";
    header += "\r\n";
    serial.writeBytes(header.c_str(), header.size());
    serial.writeBytes((const char *)request.body.second.get(), request.body.first);
    return serial;
}
