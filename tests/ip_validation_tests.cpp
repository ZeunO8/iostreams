#include <catch2/catch_test_macros.hpp>
#include <iostreams/string_is_ipv4.hpp>

TEST_CASE("string_is_ipv4 valid addresses", "[ip]")
{
    REQUIRE(iostreams::string_is_ipv4("192.168.1.1") == true);
    REQUIRE(iostreams::string_is_ipv4("0.0.0.0") == true);
    REQUIRE(iostreams::string_is_ipv4("255.255.255.255") == true);
    REQUIRE(iostreams::string_is_ipv4("10.0.0.1") == true);
    REQUIRE(iostreams::string_is_ipv4("127.0.0.1") == true);
    REQUIRE(iostreams::string_is_ipv4("1.2.3.4") == true);
}

TEST_CASE("string_is_ipv4 rejects out-of-range octets", "[ip]")
{
    REQUIRE(iostreams::string_is_ipv4("256.1.1.1") == false);
    REQUIRE(iostreams::string_is_ipv4("1.256.1.1") == false);
    REQUIRE(iostreams::string_is_ipv4("1.1.256.1") == false);
    REQUIRE(iostreams::string_is_ipv4("1.1.1.256") == false);
    REQUIRE(iostreams::string_is_ipv4("999.999.999.999") == false);
}

TEST_CASE("string_is_ipv4 rejects invalid formats", "[ip]")
{
    REQUIRE(iostreams::string_is_ipv4("") == false);
    REQUIRE(iostreams::string_is_ipv4("192.168.1") == false);
    REQUIRE(iostreams::string_is_ipv4("192.168.1.1.1") == false);
    REQUIRE(iostreams::string_is_ipv4("abc.def.ghi.jkl") == false);
    REQUIRE(iostreams::string_is_ipv4("192.168.1.") == false);
    REQUIRE(iostreams::string_is_ipv4(".192.168.1.1") == false);
    REQUIRE(iostreams::string_is_ipv4("192.168.1.1a") == false);
}

TEST_CASE("string_is_ipv4 rejects segments too long", "[ip]")
{
    REQUIRE(iostreams::string_is_ipv4("1234.1.1.1") == false);
    REQUIRE(iostreams::string_is_ipv4("1.1234.1.1") == false);
}
