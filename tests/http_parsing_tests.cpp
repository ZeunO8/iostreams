#include <catch2/catch_test_macros.hpp>
#include <iostreams/http_client.hpp>
#include <iostreams/http_common.hpp>
#include <iostreams/Serial.hpp>
#include <sstream>
#include <string>

using namespace iostreams::http;

TEST_CASE("extractUri parses http URL", "[http]")
{
    auto uri = http_client::extractUri("http://example.com/path/to/resource");
    REQUIRE(std::get<INDEX_EXTRACTED_SCHEME>(uri) == "http");
    REQUIRE(std::get<INDEX_EXTRACTED_SECURE>(uri) == false);
    REQUIRE(std::get<INDEX_EXTRACTED_HOST>(uri) == "example.com");
    REQUIRE(std::get<INDEX_EXTRACTED_PORT>(uri) == 80);
    REQUIRE(std::get<INDEX_EXTRACTED_PATH>(uri) == "/path/to/resource");
}

TEST_CASE("extractUri parses https URL", "[http]")
{
    auto uri = http_client::extractUri("https://secure.example.com:8443/api");
    REQUIRE(std::get<INDEX_EXTRACTED_SCHEME>(uri) == "https");
    REQUIRE(std::get<INDEX_EXTRACTED_SECURE>(uri) == true);
    REQUIRE(std::get<INDEX_EXTRACTED_HOST>(uri) == "secure.example.com");
    REQUIRE(std::get<INDEX_EXTRACTED_PORT>(uri) == 8443);
    REQUIRE(std::get<INDEX_EXTRACTED_PATH>(uri) == "/api");
}

TEST_CASE("extractUri parses http URL with explicit port", "[http]")
{
    auto uri = http_client::extractUri("http://localhost:3000/");
    REQUIRE(std::get<INDEX_EXTRACTED_SCHEME>(uri) == "http");
    REQUIRE(std::get<INDEX_EXTRACTED_SECURE>(uri) == false);
    REQUIRE(std::get<INDEX_EXTRACTED_HOST>(uri) == "localhost");
    REQUIRE(std::get<INDEX_EXTRACTED_PORT>(uri) == 3000);
    REQUIRE(std::get<INDEX_EXTRACTED_PATH>(uri) == "/");
}

TEST_CASE("extractUri parses https URL without path", "[http]")
{
    auto uri = http_client::extractUri("https://api.example.com");
    REQUIRE(std::get<INDEX_EXTRACTED_SCHEME>(uri) == "https");
    REQUIRE(std::get<INDEX_EXTRACTED_SECURE>(uri) == true);
    REQUIRE(std::get<INDEX_EXTRACTED_HOST>(uri) == "api.example.com");
    REQUIRE(std::get<INDEX_EXTRACTED_PORT>(uri) == 443);
    REQUIRE(std::get<INDEX_EXTRACTED_PATH>(uri) == "");
}

TEST_CASE("extractUri lowercases host by default", "[http]")
{
    auto uri = http_client::extractUri("http://EXAMPLE.COM/path");
    REQUIRE(std::get<INDEX_EXTRACTED_HOST>(uri) == "EXAMPLE.COM");
}

TEST_CASE("extractUri preserves host case when lowercaseHost is false", "[http]")
{
    auto uri = http_client::extractUri("http://EXAMPLE.COM/path", false);
    REQUIRE(std::get<INDEX_EXTRACTED_HOST>(uri) == "EXAMPLE.COM");
}

TEST_CASE("http_response deserialize parses status line and headers", "[http]")
{
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Content-Type: application/json\r\n";
    ss << "Content-Length: 13\r\n";
    ss << "\r\n";
    ss << "Hello, World!";
    ss.seekg(0);

    Serial serial(ss);
    http_response response;
    deserialize(serial, response);

    REQUIRE(response.protocol == "HTTP/");
    REQUIRE(response.version == "1.1");
    REQUIRE(response.statusCode == "200");
    REQUIRE(response.statusText == "OK");
    REQUIRE(response.headers["Content-Type"] == "application/json");
    REQUIRE(response.headers["Content-Length"] == "13");
    REQUIRE(response.body.first == 13);
}

TEST_CASE("http_response deserialize header values are not truncated", "[http]")
{
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "X-Custom-Header: some-value-here\r\n";
    ss << "Content-Length: 0\r\n";
    ss << "\r\n";
    ss.seekg(0);

    Serial serial(ss);
    http_response response;
    deserialize(serial, response);

    REQUIRE(response.headers["X-Custom-Header"] == "some-value-here");
}

TEST_CASE("http_request serialize produces correct format", "[http]")
{
    std::stringstream ss;
    Serial serial(ss);

    http_request request;
    request.verb = "GET";
    request.path = "/api/test";
    request.host = "example.com";
    request.headers["Accept"] = "application/json";
    request.body = {0, nullptr};

    serialize(serial, request);
    serial.synchronize();

    ss.seekg(0);
    std::string result((std::istreambuf_iterator<char>(ss)), std::istreambuf_iterator<char>());

    REQUIRE(result.find("GET /api/test HTTP/1.1\r\n") != std::string::npos);
    REQUIRE(result.find("Host: example.com\r\n") != std::string::npos);
    REQUIRE(result.find("Accept: application/json\r\n") != std::string::npos);
    REQUIRE(result.find("Content-Length: 0\r\n") != std::string::npos);
    REQUIRE(result.find("\r\n\r\n") != std::string::npos);
}
