#pragma once
#include "http_common.hpp"
#include "tcp_client.hpp"
#include <functional>
#include <thread>
namespace iostreams::http
{
    struct http_client
    {
        using ClientTuple = std::tuple<std::shared_ptr<tcp_client>, std::function<http_response()>, std::shared_ptr<std::thread>>;
        inline static std::pair<size_t, std::map<size_t, ClientTuple>> tcpClients = {0, {}};
        static ExtractedUri extractUri(const std::string& uri, bool lowercaseHost = false);
        static http_response restSync(const Verb& verb, const std::string& uri, const Headers& headers = {}, const Body& body = {});
        static size_t restAsync(const std::function<void(const http_response&)>& callback, const Verb& verb, const std::string& uri, const Headers& headers = {}, const Body& body = {});
        static bool cancelRestAsync(size_t& id);
    };
}