#include <catch2/catch_test_macros.hpp>
#include <iostreams/udp_server.hpp>
#include <iostreams/udp_client.hpp>
#include <iostreams/Serial.hpp>

using namespace iostreams;

TEST_CASE("udp_server creates successfully", "[udp]")
{
    udp_server server(19000);
}

TEST_CASE("udp_server creates with bitstream mode", "[udp]")
{
    udp_server server(19001, true);
}

TEST_CASE("udp_client creates successfully", "[udp]")
{
    udp_client client("127.0.0.1", 19006);
}

TEST_CASE("udp_client sends data without error", "[udp]")
{
    udp_client client("127.0.0.1", 19007);
    client << std::string("test data");
    client.flush();
}

TEST_CASE("udp_client sends binary data", "[udp]")
{
    udp_client client("127.0.0.1", 19008);
    client.write("binary", 6);
    client.flush();
}

TEST_CASE("udp_client sends integers", "[udp]")
{
    udp_client client("127.0.0.1", 19009);
    client << int32_t(42);
    client << int64_t(9999999999LL);
    client.flush();
}
