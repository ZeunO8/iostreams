#include <catch2/catch_test_macros.hpp>
#include <iostreams/http_client.hpp>
#include <iostreams/Serial.hpp>
#include <chrono>
#include <iostream>
#include <string>

using namespace iostreams::http;

namespace
{
    struct ScopedTimer
    {
        std::chrono::steady_clock::time_point start;
        const char* label;

        explicit ScopedTimer(const char* l) : start(std::chrono::steady_clock::now()), label(l) {}

        ~ScopedTimer()
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            std::cerr << "[http_live] " << label << " took " << elapsed << " ms\n";
        }
    };
}

TEST_CASE("https GET example.com via restSync (mirrors iOS bridge)", "[http][live]")
{
    ScopedTimer timer("https://example.com");
    http_response response = http_client::restSync("GET", "https://example.com");
    INFO("protocol=" << response.protocol);
    INFO("version=" << response.version);
    INFO("statusCode=" << response.statusCode);
    INFO("statusText=" << response.statusText);
    INFO("bodySize=" << response.body.first);
    REQUIRE(response.protocol == "HTTP/");
    REQUIRE_FALSE(response.version.empty());
    REQUIRE_FALSE(response.statusCode.empty());
    REQUIRE_FALSE(response.statusText.empty());
    REQUIRE(response.body.first > 0);
    REQUIRE(response.body.second);
}

TEST_CASE("https GET google.com via restSync (mirrors iOS bridge)", "[http][live]")
{
    ScopedTimer timer("https://google.com");
    http_response response = http_client::restSync("GET", "https://google.com");
    INFO("protocol=" << response.protocol);
    INFO("version=" << response.version);
    INFO("statusCode=" << response.statusCode);
    INFO("statusText=" << response.statusText);
    INFO("bodySize=" << response.body.first);
    REQUIRE(response.protocol == "HTTP/");
    REQUIRE_FALSE(response.version.empty());
    REQUIRE_FALSE(response.statusCode.empty());
    REQUIRE_FALSE(response.statusText.empty());
    REQUIRE(response.body.first > 0);
    REQUIRE(response.body.second);
}
