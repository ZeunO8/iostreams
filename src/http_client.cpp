#include <iostreams/http_client.hpp>
#include <iostreams/ssl_factory.hpp>
#include <iostreams/Serial.hpp>
#include <algorithm>
using namespace iostreams::http;

ExtractedUri http_client::extractUri(const std::string& uri, bool lowercaseHost)
{
    ExtractedUri extracted;
    auto& scheme = std::get<INDEX_EXTRACTED_SCHEME>(extracted);
    auto& secure = std::get<INDEX_EXTRACTED_SECURE>(extracted);
    auto& host = std::get<INDEX_EXTRACTED_HOST>(extracted);
    auto& port = std::get<INDEX_EXTRACTED_PORT>(extracted);
    auto& path = std::get<INDEX_EXTRACTED_PATH>(extracted);

    size_t pos = 0;

    auto find_delim = [&](char delim) -> size_t {
        auto found = uri.find(delim, pos);
        return found;
    };

    // Parse scheme (e.g. "http" or "https")
    size_t scheme_end = find_delim(':');
    if (scheme_end == std::string::npos)
    {
        throw std::runtime_error("http scheme not found");
    }
    scheme = uri.substr(pos, scheme_end - pos);
    std::transform(scheme.begin(), scheme.end(), scheme.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (scheme != "http" && scheme != "https")
    {
        throw std::runtime_error("http scheme not found");
    }

    secure = (scheme == "https");

    // Skip "://"
    pos = scheme_end + 1;
    if (pos + 2 <= uri.size() && uri[pos] == '/' && uri[pos + 1] == '/')
    {
        pos += 2;
    }

    // Parse host (and optional port)
    size_t host_end = uri.find('/', pos);
    std::string host_port;
    if (host_end == std::string::npos)
    {
        host_port = uri.substr(pos);
        path = "";
    }
    else
    {
        host_port = uri.substr(pos, host_end - pos);
        path = uri.substr(host_end);
    }

    size_t port_sep = host_port.find(':');
    if (port_sep == std::string::npos)
    {
        host = host_port;
        port = secure ? 443 : 80;
    }
    else
    {
        host = host_port.substr(0, port_sep);
        std::string port_str = host_port.substr(port_sep + 1);
        port = port_str.empty() ? (secure ? 443 : 80) : std::stoi(port_str);
    }

    if (lowercaseHost)
    {
        std::transform(host.begin(), host.end(), host.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    }

    return extracted;
}

http_response http_client::restSync(const Verb& verb, const std::string& uri, const Headers& headers, const Body& body)
{
    auto extracted = extractUri(uri);
    auto& host = std::get<INDEX_EXTRACTED_HOST>(extracted);
    SSL_CTX* ssl_ctx = 0;
    if (std::get<INDEX_EXTRACTED_SECURE>(extracted))
    {
        ssl_ctx = ssl_factory::createClient(host);
    }
    tcp_client tcpClient(host, std::get<INDEX_EXTRACTED_PORT>(extracted), ssl_ctx, false);
    Serial serial(tcpClient);
    http_request request;
    request.verb = verb;
    request.path = std::get<4>(extracted);
    request.host = std::get<2>(extracted);
    request.headers = headers;
    request.body = body;
    (serial << request).synchronize();
    http_response response;
    serial >> response;
    if (response.statusCode == "302")
    {
        auto& location = response.headers["Location"];
        return restSync(verb, location, headers, body);
    }
    return response;
}

size_t http_client::restAsync(const std::function<void(const http_response&)>& callback, const Verb& verb, const std::string& uri, const Headers& headers, const Body& body)
{
    auto id = ++std::get<0>(tcpClients);
    auto& clientsMap = std::get<1>(tcpClients);

    auto task = [callback, verb, uri, headers, body]() -> http_response
    {
        auto extracted = extractUri(uri);
        auto& host = std::get<INDEX_EXTRACTED_HOST>(extracted);
        SSL_CTX* ssl_ctx = 0;
        if (std::get<INDEX_EXTRACTED_SECURE>(extracted))
        {
            ssl_ctx = ssl_factory::createClient(host);
        }
        tcp_client tcpClient(host, std::get<INDEX_EXTRACTED_PORT>(extracted), ssl_ctx, false);
        Serial serial(tcpClient);
        http_request request;
        request.verb = verb;
        request.path = std::get<4>(extracted);
        request.host = std::get<2>(extracted);
        request.headers = headers;
        request.body = body;
        (serial << request).synchronize();
        http_response response;
        serial >> response;
        if (response.statusCode == "302")
        {
            auto& location = response.headers["Location"];
            response = restSync(verb, location, headers, body);
        }
        if (ssl_ctx)
            SSL_CTX_free(ssl_ctx);
        return response;
    };

    auto thread = std::make_shared<std::thread>([id, task, callback]()
    {
        http_response response = task();
        callback(response);
        auto& clientsMap = std::get<1>(tcpClients);
        clientsMap.erase(id);
    });

    ClientTuple clientTuple{nullptr, task, thread};
    clientsMap[id] = clientTuple;
    return id;
}

bool http_client::cancelRestAsync(size_t& id)
{
    auto& clientsMap = std::get<1>(tcpClients);
    auto iter = clientsMap.find(id);
    if (iter == clientsMap.end())
    {
        return false;
    }
    clientsMap.erase(iter);
    id = 0;
    return true;
}
