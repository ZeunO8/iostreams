#include <catch2/catch_test_macros.hpp>
#include <iostreams/get_local_ip.hpp>
#include <iostreams/string_is_ipv4.hpp>
#include <iostreams/resolve_host_or_ip_to_ip.hpp>
#include <iostreams/populate_addr_from_ip.hpp>
#include <iostreams/socket_init.hpp>
#include <arpa/inet.h>

using namespace iostreams;

TEST_CASE("get_local_ip returns non-empty string", "[utility][ip]")
{
    std::string ip = get_local_ip();
    REQUIRE(!ip.empty());
}

TEST_CASE("get_local_ip returns valid IPv4 format", "[utility][ip]")
{
    std::string ip = get_local_ip();
    REQUIRE(string_is_ipv4(ip) == true);
}

TEST_CASE("string_is_ipv4 accepts valid IPv4 addresses", "[utility][ip]")
{
    REQUIRE(string_is_ipv4("127.0.0.1") == true);
    REQUIRE(string_is_ipv4("192.168.1.1") == true);
    REQUIRE(string_is_ipv4("10.0.0.1") == true);
    REQUIRE(string_is_ipv4("255.255.255.255") == true);
    REQUIRE(string_is_ipv4("0.0.0.0") == true);
    REQUIRE(string_is_ipv4("1.2.3.4") == true);
}

TEST_CASE("string_is_ipv4 rejects invalid formats", "[utility][ip]")
{
    REQUIRE(string_is_ipv4("") == false);
    REQUIRE(string_is_ipv4("not.an.ip") == false);
    REQUIRE(string_is_ipv4("256.1.1.1") == false);
    REQUIRE(string_is_ipv4("1.1.1") == false);
    REQUIRE(string_is_ipv4("1.1.1.1.1") == false);
    REQUIRE(string_is_ipv4("abc.def.ghi.jkl") == false);
    REQUIRE(string_is_ipv4("1.1.1.1.") == false);
    REQUIRE(string_is_ipv4(".1.1.1.1") == false);
    REQUIRE(string_is_ipv4("1.1.1.999") == false);
}

TEST_CASE("string_is_ipv4 rejects IPv6", "[utility][ip]")
{
    REQUIRE(string_is_ipv4("::1") == false);
    REQUIRE(string_is_ipv4("fe80::1") == false);
    REQUIRE(string_is_ipv4("2001:db8::1") == false);
}

TEST_CASE("resolve_host_or_ip_to_ip resolves localhost", "[utility][dns]")
{
    std::string ip = resolve_host_or_ip_to_ip("localhost");
    REQUIRE(!ip.empty());
    REQUIRE(string_is_ipv4(ip) == true);
}

TEST_CASE("resolve_host_or_ip_to_ip passes through valid IP", "[utility][dns]")
{
    std::string ip = resolve_host_or_ip_to_ip("127.0.0.1");
    REQUIRE(ip == "127.0.0.1");
}

TEST_CASE("resolve_host_or_ip_to_ip resolves example.com", "[utility][dns]")
{
    std::string ip = resolve_host_or_ip_to_ip("example.com");
    REQUIRE(!ip.empty());
    REQUIRE(string_is_ipv4(ip) == true);
}

TEST_CASE("populate_addr_from_ip populates sockaddr_in", "[utility][addr]")
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    REQUIRE(addr.sin_family == AF_INET);
    REQUIRE(addr.sin_port == 0);

    uint32_t ip_int = ntohl(addr.sin_addr.s_addr);
    REQUIRE(ip_int == 0x7F000001);
}

TEST_CASE("populate_addr_from_ip with different IP", "[utility][addr]")
{
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    inet_pton(AF_INET, "192.168.1.100", &addr.sin_addr);
    REQUIRE(addr.sin_family == AF_INET);

    uint32_t ip_int = ntohl(addr.sin_addr.s_addr);
    REQUIRE(ip_int == 0xC0A80164);
}

TEST_CASE("socket_init initializes without error", "[utility][socket]")
{
    socket_init::initialize();
    REQUIRE(socket_init::initialized == true);
}

TEST_CASE("socket_init is idempotent", "[utility][socket]")
{
    socket_init::initialize();
    REQUIRE(socket_init::initialized == true);
    socket_init::initialize();
    REQUIRE(socket_init::initialized == true);
}
